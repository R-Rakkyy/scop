#ifndef SCOP_FILE_MANAGEMENT_H
#define SCOP_FILE_MANAGEMENT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*

 * File management & user input, side note: (known issue)
 *
 * I originally wanted to control a bit more the flow of upcoming files,
 * by that, i mean that i wanted to control and regulate the size,amount and type
 * of the file that the user was allowed to load and use.
 * 
 * I wanted to do a safe program that would not parse/load something that could
 * eventually make the program crash (or the whole machine). I also wantd to
 * implement a texture shrinking algorithm that would reduce the size of the texture when we could
 * Which i did, but ended up removing it and make the following decision:
 * 
 *  - The user, especially in this studying graphic pipeline project, is responsible for the files he loads and use.
 *  - Nobody while ever use this program, so losing too much time on an 
 *      implementation that would bring me no mental benefit is not worth it.
 *  - Whereas i will eventually use this project as a base for future projects, i will take care of it.
 * 
 * The same descision was made for the user input, especially the dangerous query of the GPU to chose.

*/

/**
 * @brief Read the contents of a file into a buffer.
 * @param path Path to the file.
 * @param out_buf Pointer to the buffer to store the file contents.
 * @param out_size Pointer to the size of the buffer.
 * @return 1 on success, 0 on failure.
 * @attention buffer must be freed, obviously.
 */
int fm_read_file(const char *path, void **out_buf, size_t *out_size);

#ifdef __cplusplus
}
#endif

#endif // SCOP_FILE_MANAGEMENT_H
