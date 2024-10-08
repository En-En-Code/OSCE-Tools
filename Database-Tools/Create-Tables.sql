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

-- A list of version control systems used by open source project
CREATE SEQUENCE vcs_id_seq AS int;
CREATE TABLE vcs (
    vcs_id      int PRIMARY KEY DEFAULT nextval('vcs_id_seq'),
    vcs_name    varchar(4) NOT NULL    -- The name of the version control system
);
ALTER SEQUENCE vcs_id_seq OWNED BY vcs.vcs_id;
INSERT INTO vcs (vcs_name) VALUES
    ('rhv'),    -- Code found on an archive (e.g. Wayback Machine, Google Code)
    ('n/a'),    -- Non-archive source code not hosted with a VCS.
    ('git'),
    ('svn'),
    ('cvs');

-- A list of programming languages engines have been written in.
CREATE SEQUENCE code_lang_id_seq AS int;
CREATE TABLE code_lang (
    code_lang_id    int PRIMARY KEY DEFAULT nextval('code_lang_id_seq'),
    code_lang_name  varchar(32) NOT NULL
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
-- The identifiers are the SPDX identifiers found at https://spdx.org/licenses/ ,
-- except for 'All-Rights-Reserved', 'Custom', and 'None'.
CREATE SEQUENCE license_id_seq AS int;
CREATE TABLE license (
    license_id      int PRIMARY KEY DEFAULT nextval('license_id_seq'),
    license_name    varchar(64) NOT NULL
);
ALTER SEQUENCE license_id_seq OWNED BY license.license_id;
INSERT INTO license (license_name) VALUES
    ('0BSD'),
    ('AGPL-3.0-only'),
    ('AGPL-3.0-or-later'),
    ('All-Rights-Reserved'),    -- Only includes explicitly made copyright claims.
    ('Apache-2.0'),
    ('BSD-2-Clause'),
    ('BSD-3-Clause'),
    ('CC-BY-3.0'),
    ('CC-BY-4.0'),
    ('CC0-1.0'),                -- Also includes anything labeled as Public Domain.
    ('Custom'),                 -- The author(s) wrote custom usage rules.
    ('EPL-2.0'),
    ('GPL-2.0-only'),
    ('GPL-2.0-or-later'),
    ('GPL-3.0-only'),
    ('GPL-3.0-or-later'),
    ('ISC'),
    ('LGPL-2.1-only'),
    ('LGPL-2.1-or-later'),
    ('LGPL-3.0-only'),
    ('LGPL-3.0-or-later'),
    ('MIT'),                    -- The Expat License specifically
    ('MPL-2.0'),
    ('None'),                   -- Unlicensed code is assumed All Rights Reserved by default.
    ('Unlicense'),
    ('WTFPL'),
    ('X11'),                    -- Another license sometimes referred to as 'MIT'.
    ('Zlib');

-- A non-exhaustive list of operating systems the engine's code can be built and run under.
CREATE SEQUENCE os_id_seq AS int;
CREATE TABLE os (
    os_id   int PRIMARY KEY DEFAULT nextval('os_id_seq'),
    os_name varchar(16) NOT NULL
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
    egtb_name   varchar(16) NOT NULL
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
    note        text,                   -- Notes which applies to every version of an engine.
    readme      text                    -- A more thorough documentation of the engine.
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
CREATE SEQUENCE engine_author_id_seq AS int;
CREATE TABLE engine_author (
    engine_author_id  int PRIMARY KEY DEFAULT nextval('engine_author_id_seq'),
    engine_id         int REFERENCES engine (engine_id),
    author_id         int REFERENCES author (author_id)
);
ALTER SEQUENCE engine_author_id_seq OWNED BY engine_author.engine_author_id;

-- A table storing information of where on the Internet the sources can be obtained from.
CREATE SEQUENCE source_id_seq AS int;
CREATE TABLE source (
    source_id   int PRIMARY KEY DEFAULT nextval('source_id_seq'),
    source_uri  text NOT NULL,          -- A URI to the source code of the engine.
    vcs_id      int REFERENCES vcs (vcs_id)
);
ALTER SEQUENCE source_id_seq OWNED BY source.source_id;

-- A table storing engine logos and their associated engines.
-- Modifying this is done with external tools.
CREATE SEQUENCE logo_id_seq AS int;
CREATE TABLE logo (
    logo_id   int PRIMARY KEY DEFAULT nextval('logo_id_seq'),
    engine_id int REFERENCES engine (engine_id),
    logo_img  bytea DEFAULT NULL
);
ALTER SEQUENCE logo_id_seq OWNED BY logo.logo_id;

-- A table containing specific VCS context for a particular version
CREATE TYPE fragment AS ENUM ('branch', 'commit', 'revnum', 'tag');
CREATE SEQUENCE revision_id_seq AS int;
CREATE TABLE revision (
    revision_id int PRIMARY KEY DEFAULT nextval('revision_id_seq'),
    source_id   int REFERENCES source (source_id),
    frag_type   fragment NOT NULL,
    frag_val    varchar(256) -- Allowed to be NULL, representing the trunk branch.
);
ALTER SEQUENCE revision_id_seq OWNED BY revision.revision_id;

-- A table relating engines to their sources.
CREATE SEQUENCE engine_source_id_seq AS int;
CREATE TABLE engine_source (
    engine_source_id  int PRIMARY KEY DEFAULT nextval('engine_source_id_seq'),
    engine_id         int REFERENCES engine (engine_id),
    source_id         int REFERENCES source (source_id)
);
ALTER SEQUENCE engine_source_id_seq OWNED BY engine_source.engine_source_id;

-- A table relating derivative engines to the engine(s) they originated from.
-- For example, one table entry could be the engine_id of Stockfish and the
-- origin_engine_id of Glaurung, since Stockfish is a derivative project of Glarung.
CREATE SEQUENCE predecessor_id_seq AS int;
CREATE TABLE predecessor (
    predecessor_id    int PRIMARY KEY DEFAULT nextval('predecessor_id_seq'),
    engine_id         int REFERENCES engine (engine_id),
    origin_engine_id  int REFERENCES engine (engine_id)
);
ALTER SEQUENCE predecessor_id_seq OWNED BY predecessor.predecessor_id;

-- A table relating engines to the engine(s) they took inspiration from.
-- For example, one table entry could be the engine_id of Berserk and the origin_engine_id
-- of Ethereal, since ideas from Ethereal are uniquely reimplemented by Berserk.
-- The line between using the predecessdors and the inspirations table is not always clear-cut.
-- I tend to use my best judgement and what is contained in the engines README to decide.
CREATE SEQUENCE inspiration_id_seq AS int;
CREATE TABLE inspiration (
    inspiration_id      int PRIMARY KEY DEFAULT nextval('inspiration_id_seq'),
    engine_id           int REFERENCES engine (engine_id),
    origin_engine_id    int REFERENCES engine (engine_id)
);
ALTER SEQUENCE inspiration_id_seq OWNED BY inspiration.inspiration_id;

-- A table relating engines to all of their versions and their properties.
-- Kept seperate from engines since there are an unknown amount of versions.
-- Some engines might change language, such as Prophet, which converted from C++ to C.
-- Some engines might also change license, such as moving from GPL-2.0 to GPL-3.0.
-- Status of protocol(s) and OS compatability may also change between versions.
CREATE SEQUENCE version_id_seq AS int;
CREATE TABLE version (
    version_id    int PRIMARY KEY DEFAULT nextval('version_id_seq'),
    engine_id     int REFERENCES engine (engine_id),
    version_name  varchar(255) NOT NULL,  -- The version number, e.g. 1.0, TCEC_v2, origin/HEAD.
    revision_id   int REFERENCES revision (revision_id),
    release_date  date NOT NULL,          -- Release date/date of last commit (for origin/HEAD).
    code_lang_id  int REFERENCES code_lang (code_lang_id),
    license_id    int REFERENCES license (license_id),
    is_xboard     bool NOT NULL,          -- Can the program interface with Xboard?
    is_uci        bool NOT NULL,          -- Can the program interface with UCI?
    note          text,                   -- Custom documentation/notes.
    pkgbuild      text,                   -- A file which can build the engine from source.
    UNIQUE (engine_id, version_name)
);
ALTER SEQUENCE version_id_seq OWNED BY version.version_id;

-- A table relating engines to the operating systems they can be built and ran on.
-- The contents come entirely from my own testing, so Linux will be the main representative.
CREATE SEQUENCE version_os_id_seq AS int;
CREATE TABLE version_os (
    version_os_id int PRIMARY KEY DEFAULT nextval('version_os_id_seq'),
    version_id    int REFERENCES version (version_id),
    os_id         int REFERENCES os (os_id)
);
ALTER SEQUENCE version_os_id_seq OWNED BY version_os.version_os_id;

-- A table relating engine version to the endgame tablebases they are compatible with.
-- Some engines support multiple EGTB, and many support none at all, so it best fits here.
CREATE SEQUENCE version_egtb_id_seq AS int;
CREATE TABLE version_egtb (
    version_egtb_id int PRIMARY KEY DEFAULT nextval('version_egtb_id_seq'),
    version_id  int REFERENCES version (version_id),
    egtb_id     int REFERENCES egtb (egtb_id)
);
ALTER SEQUENCE version_egtb_id_seq OWNED BY version_egtb.version_egtb_id;
