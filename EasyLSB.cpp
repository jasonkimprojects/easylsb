// BitmapParser already includes iostream, string, and vector
#include "bitmapparser.h"

class EasyLSB : public BitmapParser {
private:
    /*
    Iterator-ish object for retrieving and changing channel data.
    Supports read, increment, and reassignment of channels.
    */
    class ChannelAccessor {
    private:
        enum class Color { RED, GREEN, BLUE };
        EasyLSB& e;
        size_t row;
        size_t col;
        size_t wraparounds;
        Color color;
        uint8_t* ptr;

    public:
        ChannelAccessor(EasyLSB& easy);
        uint8_t get_channel();
        void replace_channel(uint8_t new_value);
        void next_channel();
        size_t get_wraparounds();
        uint8_t wrap_mask(size_t wraparounds);
        void print_data() {
            std::cout << "Row " << row << " Col " << col << "\n";
        }
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
    EasyLSB(const char* filename_in);
    /*
    Encodes length, then message in least significant bit,
    then inside more and more significant bits depending
    on the message.
    */
    void encode();
    // Decodes a message into msg.
    //void decode();
};

EasyLSB::EasyLSB(const char* message, const char* filename_in,
    const char* filename_out)
    : BitmapParser(filename_in), outfile(filename_out),
    msg(message), c(*this) {
    // Check compatibility first.
    check_size();
}

EasyLSB::EasyLSB(const char* filename_in)
    : BitmapParser(filename_in), outfile(nullptr),
    msg(""), c(*this) {
    check_size();
}

void EasyLSB::check_size() {
    /*
    Make sure the image is large enough for the message.
    Steganography starts with 2 bytes (16 bits) for original
    message length in bytes, so original messsage can be up to
    2^16 - 1 chars = 65535 chars in length.
    */
    if (msg.length() * BITS_PER_BYTE + NUM_LENGTH_BITS >
        infoheader().width * infoheader().height *
        BITS_PER_BYTE) {
        throw std::runtime_error(
            "Image is not large enough to hold message!\n");
    }
    else if (msg.length() > MAX_MSG_LENGTH) {
        throw std::runtime_error(
            "Message length exceeds maximum of 65535 chars!\n");
    }
}

void EasyLSB::encode() {
    // Mask to grab the least significant bit from a byte.
    const uint8_t MASK = 0b00000001;
    /*
    Get the message length and split its bits first.
    The first 16 least significant bits (2 bytes) of the image
    will be the length of the message in chars = bytes, since
    sizeof(char) = 1 byte by standards.
    */
    uint16_t len = msg.length();
    for (int shift = NUM_LENGTH_BITS - 1; shift >= 0; --shift) {
        /*
        Extract the bits of the message by masking with 0b00000001.
        Shift right 7 bits for the most significant bit of the char,
        6 bits for 2nd most significant, and so on.
        */
        uint8_t encoding_bit = ((len >> shift) & MASK);
        c.replace_channel(c.get_channel() & c.wrap_mask(c.get_wraparounds()) | encoding_bit);
        c.next_channel();
    }
    // Continue splitting bits for the chars in the message.
    for (char letter : msg) {
        std::cout << letter << "\n";
        for (int shift = BITS_PER_BYTE - 1; shift >= 0; --shift) {
            // Shift again by # of wraparounds to get it in the right place.
            uint8_t encoding_bit = ((letter >> shift) & MASK) << c.get_wraparounds();
            c.replace_channel((c.get_channel() & c.wrap_mask(c.get_wraparounds())) | encoding_bit);
            c.next_channel();
        }
    }
    // Output to file.
    save(outfile);
}

uint8_t EasyLSB::ChannelAccessor::wrap_mask(size_t wraparounds) {
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
}

EasyLSB::ChannelAccessor::ChannelAccessor(EasyLSB& easy)
    : e(easy), row(0), col(0), wraparounds(0), color(Color::RED),
    ptr(&(easy.pixels()[row][col].red)) {}

uint8_t EasyLSB::ChannelAccessor::get_channel() {
    return *ptr;
}

void EasyLSB::ChannelAccessor::replace_channel(uint8_t new_value) {
    *ptr = new_value;
}

void EasyLSB::ChannelAccessor::next_channel() {
    switch (color) {
    case Color::RED:
        // Get the green channel of the current pixel.
        color = Color::GREEN;
        ptr = &(e.pixels()[row][col].green);
        break;

    case Color::GREEN:
        // Get the blue channel of the current pixel.
        color = Color::BLUE;
        ptr = &(e.pixels()[row][col].blue);
        break;

    case Color::BLUE:
        /*
        Need to get the next pixel!
        Case 1: if there is a pixel to the right, get it.
        */
        if (col < e.pixels()[0].size() - 1) {
            ++col;
        } 
        else {
            /*
            Case 2: if there is no pixel to the right but
            a pixel to the bottom, get the bottom left pixel.
            */
            if (row < e.pixels().size() - 1) {
                ++row;
                col = 0;
            }
            else {
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
        print_data();
        ptr = &(e.pixels()[row][col].red);
        break;
    }
}

size_t EasyLSB::ChannelAccessor::get_wraparounds() {
    return wraparounds;
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
    // Encode must have argc = 5.
    if ((mode == "-e" || mode == "--encode") && (argc != 5)) {
        std::cout << "Incorrect number of arguments for encoding!\n" <<
            get_help;
        return -1;
    }
    // Decode must have argc = 3.
    else if ((mode == "-d" || mode == "--decode") && (argc != 3)) {
        std::cout << "Incorrect number of arguments for decoding!\n" <<
            get_help;
        return -1;
    }
    // Help must have argc = 2.
    else if ((mode == "-h" || mode == "--help") && (argc != 2)) {
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
    }
    // Encode or decode
    else if (mode == "-e" || mode == "--encode") {
        EasyLSB steg(argv[2], argv[3], argv[4]);
        steg.encode();
    }
    else {
        EasyLSB unsteg(argv[2]);
    }

    BitmapParser bp("steg.bmp");
    bp.print_pixels(true);

    // Todo: or with rgb values.
}

// TODO: try scrapping bit array of uint8_ts because they
// consume 8x needed space.