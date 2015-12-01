/* Copyright 2015 University of Szeged.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "js-parser.h"

int main (int argc, char* argv[])
{
  uint8_t *input;
  FILE *input_file;
  long int input_size;
  parser_error_location error_location;
  cbc_compiled_code_t *compiled_code;

  if (argc <= 1)
  {
    fprintf (stderr, "File name required!\n");
    return -1;
  }

  input_file = fopen (argv[1], "rb");
  if (!input_file)
  {
    fprintf (stderr, "Cannot open '%s'\n", argv[1]);
    return -1;
  }

  fseek (input_file, 0, SEEK_END);
  input_size = ftell (input_file);
  fseek (input_file, 0, SEEK_SET);

  input = (uint8_t *) malloc ((size_t) input_size);
  if (!input)
  {
    fclose (input_file);
    fprintf (stderr, "Cannot allocate memory\n");
    return -1;
  }

  if (fread (input, 1, (size_t) input_size, input_file) == (size_t) input_size)
  {
    fprintf (stderr, "File '%s' is loaded (size: %ld bytes)\n", argv[1], input_size);

    compiled_code = parser_parse_script (input, (size_t) input_size, &error_location);

    if (compiled_code == NULL)
    {
      printf ("Parse error '%s' at line: %d col: %d\n",
              parser_error_to_string (error_location.error),
              (int) error_location.line,
              (int) error_location.column);
    }
    else
    {
      free (compiled_code);
    }
  }
  else
  {
    fprintf (stderr, "Cannot read '%s'\n", argv[1]);
  }

  free (input);
  fclose (input_file);

  return 0;
} /* main */
