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
    'All Rights Reserved'   -- Only includes explicitly made copyright claims
    'Apache-2.0',
    'BSD-0-Clause',
    'BSD-2-Clause',
    'BSD-3-Clause',
    'CC0-1.0',              -- Also includes anything labeled as Public Domain
    'CC BY-3.0',
    'CC BY-4.0',
    'Custom',               -- The author(s) wrote custom usage rules
    'EPL-2.0',
    'Expat (MIT)',
    'GPL-2.0',
    'GPL-3.0',
    'ISC',
    'LGPL-2.1',
    'LGPL-3.0',
    'MPL-2.0',
    'None',                 -- No license is considered All Rights Reserved
    'Unlicense',
    'WTFPL',
    'Zlib'
);

-- A table with the primary purpose of relating an engine's name to the rest of the data.
CREATE TABLE engines (
    engine_id   int,            -- The key, a unique integer identifier for a engine.
    engine_name varchar(255)    -- Name of the engine, usually unique, though not guaranteed.
);

-- A table relating engines to their authors.
-- Some engines have many authors (Lc0, Stockfish, etc.), this helps achieve 1st normal form.
CREATE TABLE authors (
    engine_id   int,
    author_name	varchar(255)    -- The name of one (1) author of the engine with id engine_id.
);

-- A table relating engines to URIs where someone can access their code.
-- Kept separate from engines since code mirrors are possible, along with old code archives.
CREATE TABLE sources (
    engine_id   int,
    code_link	text    -- A URI to the source code of the engine.
);

-- A table relating derivative engines to the engine(s) they originated from.
-- For example, one table entry could be the engine_id of Stockfish and the
-- parent_engine_id of Glaurung, since Stockfish is a derivative project of Glarung.
CREATE TABLE predecessors (
    engine_id           int,
    parent_engine_id    int	    -- The engine_id of the engine engine_id takes significant work.
);

-- A table relating engines to the engine(s) they toke inspiration from.
-- For example, one table entry could be the engine_id of Berserk and the parent_engine_id
-- of Ethereal, since ideas from Ethereal are uniquely reimplemented by Berserk.
-- The line between using the predecessdors and the inspirations table is not always clear-cut.
-- I tend to use my best judgement and what is contained in the engines README to decide.
CREATE TABLE inspirations (
    engine_id           int,
    parent_engine_id    int     -- The engine id of the engine the engine takes inspiration from.
);

-- A table relating engines to all of their versions and their properties.
-- Kept seperate from engines since there are an unknown amount of versions.
-- Some engines might change language, such as Prophet, which converted from C++ to C.
-- Some engines might also change license, such as moving from GPL-2.0 to GPL-3.0.
-- Status of protocol(s) and OS compatability may also change.
CREATE TABLE versions (
    engine_id       int,
    version_num	    varchar(255),   -- The version identifier, such as 1.0 or TCEC_v2.
    release_date    date,           -- The release date, or date of last modification (for dev).
    program_lang    program_lang,   -- The programming language the version was written in.
    license         license,        -- The license, if any, the program was released under.
    accepts_xboard  bool,           -- Whether the program interfaces with the Xboard protocol.
    accepts_uci     bool,           -- Whether the program interfaces with the UCI protocol.
    linux_compat    bool,           -- Whether I have built and run the engine for Linux (my OS).
    notes           text            -- Custom documentation for things I believe are of note.
);
