/*
 * Base64 library
 *
 * Copyright (C) 2017 Ahmed Elzoughby (https://github.com/elzoughby/Base64)
 * Copyright (C) 2020 Brandenburg University of Technology Cottbus - Senftenberg (BTU-CS)
 *
 * Brandenburg University of Technology Cottbus - Senftenberg (BTU-CS)
 * Department of Computer Science, Information and Media Technology
 * Computer Networks and Communication Systems Group
 *
 * Practical course "Introduction to Computer Networks"
 * Task 1: Implementation of a Simple SMTP Client
 *
 * @author: Sebastian Boehm <sebastian.boehm@b-tu.de>
 * @version: 2020-11-13
 *
 */

#ifndef BASE46_H
#define BASE46_H

#include <stdlib.h>
#include <memory.h>

/**
 * Encodes ASCII string into base64 format string
 *
 * @param plain ASCII string to be encoded
 * @return encoded base64 format string
 */
char* base64_encode(char* plain);

/**
 * Decodes base64 format string into ASCII string
 *
 * @param cipher encoded base64 format string
 * @return decoded ASCII string
 */
char* base64_decode(char* cipher);

#endif /* BASE64_H */
