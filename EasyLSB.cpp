// Copyright 2019 Jason Kim. All rights reserved.
/*
EasyLSB.cpp

An encoder and decoder for messages hidden inside bitmap
images by least significant bit steganography. Uses my 
BitmapParser library.

The first 16 least significant bits is the length field
before the actual message bits. Length is expressed in
chars of the message (i.e. bytes of the message since 
sizeof(char) = 1 by standard). 

The style confroms to Google's coding style guide for C++,
and has been checked with cpplint.

Jason Kim
June 29, 2019
*/

// Already includes iostream, string, and vector.
#include "bitmapparser.h"

/*
EasyLSB class, extending from BitmapParser.
Inheritance allows easier addition of the channel accessor.
*/
class EasyLSB : public BitmapParser {
 private:
    /*
    The channel accessor is an iterator-like object
    for retrieving and changing channel data.
    Supports read, increment, and reassignment of channels.
    */
    class ChannelAccessor {
     private:
        // Only 3 possible colors, so use an enum class.
        enum class Color { RED, GREEN, BLUE };
        // Need an instance of EasyLSB to access.
        EasyLSB* e;
        size_t row;
        size_t col;
        /*
        If the message cannot fit in the least significant bits
        of all the channels, it will wrap around the pixels array
        and be stored in the 2nd least significant,
        3rd least... all the way up to the most significant (8th) bit.
        This variable counts the number of wraps.
        */
        size_t wraparounds;
        Color color;
        // Pointer to color channel.
        uint8_t* ptr;

     public:
        // Constructor - makes accessor point to red channel of row 0, col 0.
        explicit ChannelAccessor(EasyLSB* easy);
        // Channel accessor, mutator, increment.
        uint8_t get_channel();
        void replace_channel(uint8_t new_value);
        void next_channel();
        // Accessor for getting number of wraps.
        size_t get_wraparounds();
        // Returns the appropriate mask for each wrap.
        uint8_t wrap_mask(size_t wraparounds);
        // Returns the appropriate bitmask for each wrap round.
        uint8_t bitmask(size_t wraparounds);
    };
    // Constants for readability
    const size_t BITS_PER_BYTE = 8;
    const size_t NUM_LENGTH_BITS = 16;
    const size_t MAX_MSG_LENGTH = 65535;
    /*
    No need to remember input file name because it's passed
    directly to superclass BitmapParser, but we need to hold
    on to the output file in order to call BitmapParser::save()
    when we are done with stego.
    If mode is decode, this is nullptr.
    */
    const char *outfile;
    /*
    Message from command line args, if encode.
    If mode is decode, it starts out as an empty string.
    The decoded message will be stored here.
    */
    std::string msg;
    // For keeping track of which channels we are at.
    ChannelAccessor c;
    // Helper function for constructor.
    void check_size();

 public:
    // Constructor for encode.
    EasyLSB(const char* message, const char* filename_in,
        const char* filename_out);
    // Constructor for decode.
    explicit EasyLSB(const char* filename_in);
    /*
    Encodes length, then message in least significant bit,
    then inside more and more significant bits depending
    on the message.
    */
    void encode();
    // Decodes a message into msg.
    void decode();
};

// Encode constructor
EasyLSB::EasyLSB(const char* message, const char* filename_in,
    const char* filename_out)
    : BitmapParser(filename_in), outfile(filename_out),
    msg(message), c(this) {
    // Check compatibility first.
    check_size();
}

// Decode constructor - leave outfile and msg blank.
EasyLSB::EasyLSB(const char* filename_in)
    : BitmapParser(filename_in), outfile(nullptr),
    msg(""), c(this) {
    // Check compatibility first.
    check_size();
}

/*
Make sure the image is large enough for the message.
Steganography starts with 2 bytes (16 bits) for original
message length in bytes, so original messsage can be up to
2^16 - 1 chars = 65535 chars in length.
*/
void EasyLSB::check_size() {
    if (msg.length() * BITS_PER_BYTE + NUM_LENGTH_BITS >
        infoheader().width * infoheader().height *
        BITS_PER_BYTE) {
        throw std::runtime_error(
            "Image is not large enough to hold message!\n");
    } else if (msg.length() > MAX_MSG_LENGTH) {
        throw std::runtime_error(
            "Message length exceeds maximum of 65535 chars!\n");
    }
}

/*
Encodes a message inside the bitmap image.
First 16 LSBs is the length of the message in chars = bytes.
Then each channel's LSB is overwritten in R,G,B order within a pixel,
and left to right, top to bottom for the pixels vector.
If the message is larger, the accessor rolls over to the red channel
of _pixels[0][0] but writes the 2nd least significant bit this time.
At the extreme case this will overwrite the most significant bit of the
blue channel of the bottom right pixel of the image.
*/
void EasyLSB::encode() {
    // Mask to grab the least significant bit from a byte.
    const uint8_t MASK = 0b00000001;
    // Complete the 16 bit length field.
    uint16_t len = msg.length();
    for (int shift = NUM_LENGTH_BITS - 1; shift >= 0; --shift) {
        /*
        Extract the bits of the message by masking with 0b00000001.
        Shift right 7 bits for the most significant bit of the char,
        6 bits for 2nd most significant, and so on.
        */
        uint8_t encoding_bit = ((len >> shift) & MASK);
        // Replaces just at the location of the encoding bit.
        c.replace_channel((c.get_channel() &
            c.wrap_mask(c.get_wraparounds())) | encoding_bit);
        // Advance to the next channel.
        c.next_channel();
    }
    // Continue splitting bits for the chars in the message.
    for (char letter : msg) {
        for (int shift = BITS_PER_BYTE - 1; shift >= 0; --shift) {
            // Shift again by # of wraparounds to get it in the right place.
            uint8_t encoding_bit =
                ((letter >> shift) & MASK) << c.get_wraparounds();
            // Replaces just at the location of the encoding bit.
            c.replace_channel((c.get_channel() &
                c.wrap_mask(c.get_wraparounds())) | encoding_bit);
            // Advance to the next channel.
            c.next_channel();
        }
    }
    // Length and msg encoded. Output the result.
    save(outfile);
}

/*
Attempts to decode a message within a bitmap image using the reverse
method of what encode() does. First reads the 16 LSBs for length, and proceeds
to fuse 8 LSBs (or nth least significant if there is wraparound)
into one char until the length is reached. Result is stored in msg.
Note that running this on a regular bitmap image will most likely
result in gibberish or no output.
*/
void EasyLSB::decode() {
    // Extract the length first.
    uint16_t len = 0;
    for (size_t i = 0; i < NUM_LENGTH_BITS; ++i) {
        /*
        Wraparounds is zero at start, but just for readability.
        Isolates the least significant bit of current channel.
        */
        uint16_t bit = c.get_channel() & c.bitmask(c.get_wraparounds());
        /*
        First lsb must be shifted 15 left, second lsb shifted 14 left...
        and the 16th lsb should not be shifted.
        */
        bit = bit << (NUM_LENGTH_BITS - 1 - i);
        // Add this bit to build up len.
        len = len | bit;
        // Advance to the next channel.
        c.next_channel();
    }
    // There are len chars = len * 8 bits in msg.
    for (size_t i = 0; i < len; ++i) {
        // Empty byte to be filled in.
        unsigned char char_byte = 0;
        // Assemble the next 8 bits.
        for (size_t j = 0; j < BITS_PER_BYTE; ++j) {
            /*
            Isolate nth least sig bit of current channel, where n
            is the number of wraparounds.
            Shift by # of wraparounds to bring it to lsb position.
            */
            unsigned char bit = (c.get_channel() &
                c.bitmask(c.get_wraparounds())) >> c.get_wraparounds();
            // Shift based on order in the byte.
            bit = bit << (BITS_PER_BYTE - 1 - j);
            // Add this bit to build char_byte.
            char_byte = char_byte | bit;
            // Advance to the next channel.
            c.next_channel();
        }
        // Append this char to msg.
        msg += char_byte;
    }
    // Output the result.
    std::cout << msg << std::endl;
}

/*
Returns the appropriate bit mask for setting individual bits,
depending on how many times the message has wrapped around the pixels.
*/
inline uint8_t EasyLSB::ChannelAccessor::wrap_mask(size_t wraparounds) {
    switch (wraparounds) {
    case 0:
        return 0b11111110;
    case 1:
        return 0b11111101;
    case 2:
        return 0b11111011;
    case 3:
        return 0b11110111;
    case 4:
        return 0b11101111;
    case 5:
        return 0b11011111;
    case 6:
        return 0b10111111;
    case 7:
        return 0b01111111;
    }
    // So the compiler doesn't complain
    return 0;
}

/*
Returns the appropriate bit mask for isolating individual bits
from a channel, depending on how many wraps (rollovers) were
used in the decoding process.
*/
inline uint8_t EasyLSB::ChannelAccessor::bitmask(size_t wraparounds) {
    switch (wraparounds) {
    case 0:
        return 0b00000001;
    case 1:
        return 0b00000010;
    case 2:
        return 0b00000100;
    case 3:
        return 0b00001000;
    case 4:
        return 0b00010000;
    case 5:
        return 0b00100000;
    case 6:
        return 0b01000000;
    case 7:
        return 0b10000000;
    }
    // So the compiler doesn't complain
    return 0;
}

// Constructor for channel accessor - ptr set to red channel of first pixel.
EasyLSB::ChannelAccessor::ChannelAccessor(EasyLSB* easy)
    : e(easy), row(0), col(0), wraparounds(0), color(Color::RED),
    ptr(&(easy->pixels()[row][col].red)) {}

// Accessor for getting the channel value.
inline uint8_t EasyLSB::ChannelAccessor::get_channel() {
    return *ptr;
}

// Mutator for replacing the channel value.
inline void EasyLSB::ChannelAccessor::replace_channel(uint8_t new_value) {
    *ptr = new_value;
}

// Accessor for number of wraps that msg has done around pixels.
inline size_t EasyLSB::ChannelAccessor::get_wraparounds() {
    return wraparounds;
}

// "Increments" to the next channel.
void EasyLSB::ChannelAccessor::next_channel() {
    switch (color) {
    case Color::RED:
        // Get the green channel of the current pixel.
        color = Color::GREEN;
        ptr = &(e->pixels()[row][col].green);
        break;

    case Color::GREEN:
        // Get the blue channel of the current pixel.
        color = Color::BLUE;
        ptr = &(e->pixels()[row][col].blue);
        break;

    case Color::BLUE:
        /*
        Need to get the next pixel!
        Case 1: if there is a pixel to the right, get it.
        */
        if (col < e->pixels()[0].size() - 1) {
            ++col;
        } else {
            /*
            Case 2: if there is no pixel to the right but
            a pixel to the bottom, get the bottom left pixel.
            */
            if (row < e->pixels().size() - 1) {
                ++row;
                col = 0;
            } else {
                /*
                Case 3: if we reached the end of the image
                (bottom right pixel), loop around and go to the
                top left pixel.
                */
                row = 0;
                col = 0;
                // Need to increment wraparound.
                ++wraparounds;
            }
        }
        // Start with red channel again.
        color = Color::RED;
        ptr = &(e->pixels()[row][col].red);
        break;
    }
}

/*
Usage:

1. For encoding a message inside an image:
EasyLSB <-e or --encode> <message> <image filename> <output filename>

2. For decoding a message from a LSB encoded image:
EasyLSB <-d or --decode> <image filename>

3. To display help message:
EasyLSB <-h or --help>

*/
int main(int argc, char *argv[]) {
    // Repeated many times, so save it.
    std::string get_help = "Run EasyLSB <-h or --help> for information.\n";
    // Check for number of arguments.
    if (!(argc == 5 || argc == 3 || argc == 2)) {
        std::cout << "Incorrect number of arguments!\n" << get_help;
        return -1;
    }
    // Construct std::string for better comparison.
    std::string mode(argv[1]);
    // Check that mode is valid.
    if (!(mode == "-e" || mode == "--encode" ||
        mode == "-d" || mode == "--decode" ||
        mode == "-h" || mode == "--help")) {
        std::cout << "Incorrect mode!\n" << get_help;
        return -1;
    }
    /*
    Encode must have argc = 5.
    Decode must have argc = 3.
    Help must have argc = 2.
    */
    if ((mode == "-e" || mode == "--encode") && (argc != 5)) {
        std::cout << "Incorrect number of arguments for encoding!\n" <<
            get_help;
        return -1;
    } else if ((mode == "-d" || mode == "--decode") && (argc != 3)) {
        std::cout << "Incorrect number of arguments for decoding!\n" <<
            get_help;
        return -1;
    } else if ((mode == "-h" || mode == "--help") && (argc != 2)) {
        std::cout << "Incorrect number of arguments for help!\n" <<
            get_help;
        return -1;
    }
    // If help, print the usage guide and terminate.
    if (mode == "-h" || mode == "--help") {
        std::cout << "Usage:\n" <<
            "EasyLSB <-e or --encode> <message>" <<
            " <image filename> <output filename>\n" <<
            "EasyLSB <-d or --decode> <image filename>\n" <<
            "EasyLSB <-h or --help>\n";
        return 0;
    } else if (mode == "-e" || mode == "--encode") {
        EasyLSB steg(argv[2], argv[3], argv[4]);
        steg.encode();
    } else {
        EasyLSB unsteg(argv[2]);
        unsteg.decode();
    }
}
