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
CREATE SCHEMA engine;
SET search_path TO engine;

-- A list of programming languages engines have been written in.
CREATE SEQUENCE code_lang_id_seq AS int;
CREATE TABLE code_lang (
    code_lang_id    int PRIMARY KEY DEFAULT nextval('code_lang_id_seq'),
    code_lang_name  varchar(64) NOT NULL
);
ALTER SEQUENCE code_lang_id_seq OWNED BY code_lang.code_lang_id;
INSERT INTO code_lang (code_lang_name) VALUES
    ('Ada'),
    ('Assembly'),       -- Includes many languages easily converted to machine code
    ('BASIC'),          -- Includes dialects like FreeBASIC and QuickBASIC
    ('Bash'),
    ('C'),
    ('C++'),
    ('C#'),
    ('Clojure'),
    ('D'),
    ('Dart'),
    ('Delphi'),
    ('Fortran'),
    ('Forth'),
    ('Free Pascal'),    -- Only includes code written specifically for the FPC
    ('Frege'),
    ('Go'),
    ('Haskell'),
    ('Java'),
    ('JavaScript'),
    ('Kotlin'),
    ('Lua'),
    ('Nim'),
    ('O''Caml'),
    ('PHP'),
    ('Python'),
    ('Ruby'),
    ('Rust'),
    ('Scheme'),
    ('Swift'),
    ('TypeScript'),     -- Includes AssemblyScript
    ('VBA'),            -- Office Visual Basic for Applications
    ('Visual Basic'),
    ('Zig');

-- A non-exhaustive list of licenses I have found code to be under.
-- The following comments are not legal advice and do not form an attorney-client relationship.
CREATE SEQUENCE license_id_seq AS int;
CREATE TABLE license (
    license_id      int PRIMARY KEY DEFAULT nextval('license_id_seq'),
    license_name    varchar(64) NOT NULL
);
ALTER SEQUENCE license_id_seq OWNED BY license.license_id;
INSERT INTO license (license_name) VALUES
    ('AGPL-3.0'),
    ('All Rights Reserved'),    -- Only includes explicitly made copyright claims.
    ('Apache-2.0'),
    ('BSD-0-Clause'),
    ('BSD-2-Clause'),
    ('BSD-3-Clause'),
    ('CC0-1.0'),                -- Also includes anything labeled as Public Domain.
    ('CC BY-3.0'),
    ('CC BY-4.0'),
    ('Custom'),                 -- The author(s) wrote custom usage rules.
    ('EPL-2.0'),
    ('Expat (MIT)'),
    ('GPL-2.0'),
    ('GPL-3.0'),
    ('ISC'),
    ('LGPL-2.1'),
    ('LGPL-3.0'),
    ('MPL-2.0'),
    ('Unlicense'),
    ('WTFPL'),
    ('Zlib');

-- A non-exhaustive list of operating systems the engine's code can be built and run under.
CREATE SEQUENCE os_id_seq AS int;
CREATE TABLE os (
    os_id   int PRIMARY KEY DEFAULT nextval('os_id_seq'),
    os_name varchar(64) NOT NULL
);
ALTER SEQUENCE os_id_seq OWNED BY os.os_id;
INSERT INTO os (os_name) VALUES
    ('Android'),
    ('Linux'),
    ('MacOS'),
    ('Windows');

-- A list of endgame tablebases (EGTB) an engine is capable of using.
CREATE SEQUENCE egtb_id_seq AS int;
CREATE TABLE egtb (
    egtb_id     int PRIMARY KEY DEFAULT nextval('egtb_id_seq'),
    egtb_name   varchar(64) NOT NULL
);
ALTER SEQUENCE egtb_id_seq OWNED BY egtb.egtb_id;
INSERT INTO egtb (egtb_name) VALUES
    ('Gaviota'),
    ('Nalimov'),
    ('Scorpio'),
    ('Syzygy');

-- A table with the primary purpose of relating an engine's name to the rest of the data.
CREATE SEQUENCE engine_id_seq AS int;
CREATE TABLE engine (
    engine_id   int PRIMARY KEY DEFAULT nextval('engine_id_seq'),
    engine_name varchar(255) NOT NULL,  -- Engine name, often, though not guaranteed, unique.
    note        text                    -- Notes which applies to every version of an engine.
);
ALTER SEQUENCE engine_id_seq OWNED BY engine.engine_id;

-- A table storing information about the authors of chess engines.
CREATE SEQUENCE author_id_seq AS int;
CREATE TABLE author (
    author_id   int PRIMARY KEY DEFAULT nextval('author_id_seq'),
    author_name varchar(255) NOT NULL   -- Author name, not guaranteed unique.
);
ALTER SEQUENCE author_id_seq OWNED BY author.author_id;

-- A table relating engines to their authors.
CREATE TABLE engine_authorship (
    engine_id   int REFERENCES engine (engine_id),
    author_id   int REFERENCES author (author_id),
    PRIMARY KEY (engine_id, author_id)
);

-- A table storing information of where on the Internet the sources can be obtained from.
CREATE SEQUENCE source_id_seq AS int;
CREATE TABLE source (
    source_id   int PRIMARY KEY DEFAULT nextval('source_id_seq'),
    source_link text NOT NULL           -- A URI to the source code of the engine.
);
ALTER SEQUENCE source_id_seq OWNED BY source.source_id;

-- A table relating engines to their sources.
CREATE TABLE source_reference (
    engine_id   int REFERENCES engine (engine_id),
    source_id   int REFERENCES source (source_id),
    PRIMARY KEY (engine_id, source_id)
);

-- A table relating derivative engines to the engine(s) they originated from.
-- For example, one table entry could be the engine_id of Stockfish and the
-- parent_engine_id of Glaurung, since Stockfish is a derivative project of Glarung.
CREATE TABLE predecessor (
    engine_id           int REFERENCES engine (engine_id),
    parent_engine_id    int REFERENCES engine (engine_id),
    PRIMARY KEY (engine_id, parent_engine_id)
);

-- A table relating engines to the engine(s) they toke inspiration from.
-- For example, one table entry could be the engine_id of Berserk and the parent_engine_id
-- of Ethereal, since ideas from Ethereal are uniquely reimplemented by Berserk.
-- The line between using the predecessdors and the inspirations table is not always clear-cut.
-- I tend to use my best judgement and what is contained in the engines README to decide.
CREATE TABLE inspiration (
    engine_id           int REFERENCES engine (engine_id),
    parent_engine_id    int REFERENCES engine (engine_id),
    PRIMARY KEY (engine_id, parent_engine_id)
);

-- A table relating engines to all of their versions and their properties.
-- Kept seperate from engines since there are an unknown amount of versions.
-- Some engines might change language, such as Prophet, which converted from C++ to C.
-- Some engines might also change license, such as moving from GPL-2.0 to GPL-3.0.
-- Status of protocol(s) and OS compatability may also change between versions.
CREATE SEQUENCE version_id_seq AS int;
CREATE TABLE version (
    version_id      int PRIMARY KEY DEFAULT nextval('version_id_seq'),
    engine_id       int REFERENCES engine (engine_id),
    version_num     varchar(255) NOT NULL,  -- The version number, e.g. 1.0, TCEC_v2, origin/HEAD.
    release_date    date NOT NULL,          -- Release date/date of last commit (for origin/HEAD).
    program_lang    int REFERENCES code_lang (code_lang_id),
    license         int REFERENCES license (license_id),
    accepts_xboard  bool NOT NULL,          -- Can the program interface with Xboard?
    accepts_uci     bool NOT NULL,          -- Can the program interface with UCI?
    note            text                    -- Custom documentation/notes.
);
ALTER SEQUENCE version_id_seq OWNED BY version.version_id;

-- A table relating engines to the operating systems they can be built and ran on.
-- The contents come entirely from my own testing, so Linux will be the main representative.
CREATE TABLE compatible_os (
    version_id  int REFERENCES version (version_id),
    os_id       int REFERENCES os (os_id),
    PRIMARY KEY (version_id, os_id)
);

-- A table relating engine version to the endgame tablebases they are compatible with.
-- Some engines support multiple EGTB, and many support none at all, so it best fits here.
CREATE TABLE compatible_egtb (
    version_id  int REFERENCES version (version_id),
    egtb_id     int REFERENCES egtb (egtb_id),
    PRIMARY KEY (version_id, egtb_id)
);
