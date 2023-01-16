/*
Copyright 2023 En-En-Code

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

-- A schema to refer to the databases by.
CREATE SCHEMA engines;
SET search_path TO engines;

-- A non-exhaustive list of programming languages I have encounted.
CREATE TYPE program_lang AS ENUM (
    'Ada',
    'Assembly',     -- Includes many languages easily converted to machine code
    'BASIC',        -- Includes dialects like FreeBASIC and QuickBASIC
    'Bash',
    'C',
    'C++',
    'C#',
    'Clojure',
    'D',
    'Dart',
    'Delphi',
    'Fortran',
    'Forth',
    'Free Pascal',  -- Only includes code written specifically for the FPC
    'Frege',
    'Go',
    'Haskell',
    'Java',
    'JavaScript',
    'Kotlin',
    'Lua',
    'Nim',
    'O''Caml',
    'PHP',
    'Python',
    'Ruby',
    'Rust',
    'Scheme',
    'Swift',
    'TypeScript',   -- Includes AssemblyScript
    'VBA',          -- Office Visual Basic for Applications
    'Visual Basic',
    'Zig'
);

-- A non-exhaustive list of licenses I have found code to be under.
-- The following comments are not legal advice and do not form an attorney-client relationship.
CREATE TYPE license AS ENUM (
    'AGPL-3.0',
    'All Rights Reserved'   -- Only includes explicitly made copyright claims.
    'Apache-2.0',
    'BSD-0-Clause',
    'BSD-2-Clause',
    'BSD-3-Clause',
    'CC0-1.0',              -- Also includes anything labeled as Public Domain.
    'CC BY-3.0',
    'CC BY-4.0',
    'Custom',               -- The author(s) wrote custom usage rules.
    'EPL-2.0',
    'Expat (MIT)',
    'GPL-2.0',
    'GPL-3.0',
    'ISC',
    'LGPL-2.1',
    'LGPL-3.0',
    'MPL-2.0',
    'None',                 -- No license is considered All Rights Reserved.
    'Unlicense',
    'WTFPL',
    'Zlib'
);

-- A non-exhaustive list of operating systems the engine's code can be built and run under.
CREATE TYPE os AS ENUM (
    'Android',
    'Linux',
    'Mac',
    'Windows'
);

-- A table with the primary purpose of relating an engine's name to the rest of the data.
CREATE SEQUENCE engine_id_seq AS int;
CREATE TABLE engines (
    engine_id   int PRIMARY KEY DEFAULT nextval('engine_id_seq'),
    engine_name varchar(255) NOT NULL   -- Engine name, often, though not guaranteed, unique.
);
ALTER SEQUENCE engine_id_seq OWNED BY engines.engine_id;

-- A table relating engines to their authors.
-- Some engines have many authors (Lc0, Stockfish, etc.), this helps achieve 1st normal form.
CREATE TABLE authors (
    engine_id   int REFERENCES engines (engine_id),
    author_name varchar(255) NOT NULL   -- The name of one author of the referenced engine.
);

-- A table relating engines to URIs where someone can access their code.
-- Kept separate from engines since code mirrors are possible, along with old code archives.
CREATE TABLE sources (
    engine_id   int REFERENCES engines (engine_id),
    code_link   text NOT NULL           -- A URI to the source code of the engine.
);

-- A table relating derivative engines to the engine(s) they originated from.
-- For example, one table entry could be the engine_id of Stockfish and the
-- parent_engine_id of Glaurung, since Stockfish is a derivative project of Glarung.
CREATE TABLE predecessors (
    engine_id           int REFERENCES engines (engine_id),
    parent_engine_id    int REFERENCES engines (engine_id)
);

-- A table relating engines to the engine(s) they toke inspiration from.
-- For example, one table entry could be the engine_id of Berserk and the parent_engine_id
-- of Ethereal, since ideas from Ethereal are uniquely reimplemented by Berserk.
-- The line between using the predecessdors and the inspirations table is not always clear-cut.
-- I tend to use my best judgement and what is contained in the engines README to decide.
CREATE TABLE inspirations (
    engine_id           int REFERENCES engines (engine_id),
    parent_engine_id    int REFERENCES engines (engine_id)
);

-- A table relating engines to all of their versions and their properties.
-- Kept seperate from engines since there are an unknown amount of versions.
-- Some engines might change language, such as Prophet, which converted from C++ to C.
-- Some engines might also change license, such as moving from GPL-2.0 to GPL-3.0.
-- Status of protocol(s) and OS compatability may also change between versions.
CREATE SEQUENCE version_id_seq AS int;
CREATE TABLE versions (
    version_id      int PRIMARY KEY DEFAULT nextval('version_id_seq'),
    engine_id       int REFERENCES engines (engine_id),
    version_num     varchar(255) NOT NULL,  -- The version number, e.g. 1.0, TCEC_v2, origin/HEAD.
    release_date    date NOT NULL,          -- Release date/date of last commit (for origin/HEAD).
    program_lang    program_lang NOT NULL,  -- Programming language the version was written in.
    license         license NOT NULL,       -- The license the program was released under.
    accepts_xboard  bool NOT NULL,          -- Can the program interface with Xboard?
    accepts_uci     bool NOT NULL,          -- Can the program interface with UCI?
    notes           text                    -- Custom documentation/notes.
);
ALTER SEQUENCE version_id_seq OWNED BY versions.version_id;

-- A table relating engines to the operating systems they can be built and ran on.
-- The contents come entirely from my own testing, so Linux will be the main representative.
CREATE TABLE compatible_systems (
    version_id  int REFERENCES versions (version_id),
    compat_os   os NOT NULL             -- One operating system the engine is compatable with.
);
