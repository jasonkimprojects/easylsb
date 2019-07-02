// Copyright 2019 Jason Kim. All rights reserved.
/*
EasyLSB.h

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

#ifndef EASYLSB_H_
#define EASYLSB_H_

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
        uint8_t get_channel() const;
        void replace_channel(uint8_t new_value);
        void next_channel();
        // Accessor for getting number of wraps.
        size_t get_wraparounds() const;
        // Returns the appropriate mask for each wrap.
        uint8_t wrap_mask(size_t wraparounds) const;
        // Returns the appropriate bitmask for each wrap round.
        uint8_t bitmask(size_t wraparounds) const;
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
    const char* outfile;
    /*
    Message from command line args, if encode.
    If mode is decode, it starts out as an empty string.
    The decoded message will be stored here.
    */
    std::string msg;
    // For keeping track of which channels we are at.
    ChannelAccessor c;
    // Helper function for constructor.
    void check_size() const;

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

#endif  // EASYLSB_H_
