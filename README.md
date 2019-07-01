# EasyLSB

A program to encode and decode messages inside bitmap images using least significant bit steganography.
For more information on the technique, visit [this link.](https://www.cybrary.it/0p3n/hide-secret-message-inside-image-using-lsb-steganography/)

This program uses my BitmapParser library, which is in another repository of mine. [Click here for BitmapParser.](https://github.com/jasonkimprojects/bitmapparser)

The inspiration for my personal project comes from the steganography mini-unit from EECS 388 (Introduction to Computer Security) at the [University of Michigan.](https://umich.edu/). Please note that this was **not** taken from any part of any academic project during my coursework. The steganography mini-unit covered the technique in theory, but students were not instructed to implement it. I found it difficult to find simple steganography programs, so I made one of my own.

I have referred to [Google's C++ Style Guide](https://google.github.io/styleguide/cppguide.html) while writing *EasyLSB* for readability. My source has been checked with `cpplint`.

Last but not least, this program is licensed under the GNU GPL v3.0.

Jason Kim  
June 30, 2019

## Usage

#### 1. For encoding a message inside an image:
`./EasyLSB <-e or --encode> <message> <bitmap image filename> <output filename>`  

`<output filename>` will be created in the same directory the program was run.
Please note that if `<bitmap image filename>` and `<output filename>` are the same,
then the input image will be **overwritten!**

#### 2. For decoding a message from a LSB encoded image:
`./EasyLSB <-d or --decode> <bitmap image filename>`  

If `<bitmap image filename>` is an image containing steganogrpahy by this program, 
	then the message will be printed to `stdout`.

#### 3. To display the help message:
`./EasyLSB <-h or --help>`

## Examples

* `./EasyLSB -e "this is a secret message" "image.bmp" "image_steg.bmp"`

Given that `image.bmp` exists in the current directory, `image.bmp` will not be modified; and `image_steg.bmp` will be created in the same directory.

* `./EasyLSB -d "image_steg.bmp"`

The text "this is a secret message" will be printed to `stdout`.

## Information

The first 16 least significant bits of the image make up the length field before the actual message bits.
Length is expressed in number of characters in the message. Since `sizeof(char)` is 1 by standards, this is
equal to the number of bytes in the message = 8 times the number of bits the message takes up.

If the message cannot fit in the least significant bits of all the channels, it will be 'looped around' the image, overwriting the second least significant bit, third least significant bit... up to the most significant 
(i.e. eighth least significant) bit. 

**This leads to three important consequences:**

* The more the message 'loops around' the image, the more the channels will be modified from the original image. This may make it easier to detect steganography in the modified image.

* The number of bits in the message plus the 16 length bits must be less or equal to the number of bits in the image, which is width * height * 8 for a 24 bit image since one channel is 8 bits.

* Because 16 bits are preallocated for length, the maximum message length is 2^16 - 1 = 65535 characters.

## Exceptions

EasyLSB can throw two `std::runtime_error` exceptions. They can be distinguished by the string returned when `what()` is called.

* `what()` will return "Image is not large enough to hold message!" if the image cannot hold the message bits plus the 16 length bits.

* `what()` will return "Message length exceeds maximum of 65535 chars!" if, trivially, the message is longer than 65535 characters.