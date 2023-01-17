# Copyright 2023 En-En-Code
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

CC := gcc
EXEC := a.out

SRC_FILES := $(wildcard *.c)
OBJ_FILES := $(patsubst %.c,%.o,$(SRC_FILES))

CFLAGS := $(CFLAGS)
CFLAGS += -Wall
CPPFLAGS := $(CPPFLAGS)
CPPFLAGS += $(foreach dir,$(shell pg_config --includedir),-I$(dir))
LIBFLAGS := $(foreach dir,$(shell pg_config --libdir),-L$(dir)) -lpq

all: $(EXEC)

$(EXEC): $(OBJ_FILES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LIBFLAGS)

%.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LIBFLAGS)

clean:
	rm -f *.o $(EXEC)