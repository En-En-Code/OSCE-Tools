# Database-Tools

A database is far better at being interacted with by other code than a spreadsheet. Therefore, I am migrating from a spreadsheet to a [PostgreSQL](https://www.postgresql.org/) database, to make features such as automatically checking repositories for updates significantly easier.

## Usage

After installing PostgreSQL, run:
* `sudo -i -u postgres` to change to the `postgres` user, 
* `systemctl start postgresql` to start the local server,
* `createdb <database-name>` to create the database, and
* `psql <database-name>` to have a direct interface to the database.
* While in the database, `CREATE ROLE <username> LOGIN CREATEDB CREATEROLE;` can also can used while the `postgres` user to create a new user under your own name with useful (but not superuser) permissions.

More likely, however, you will want a way to call the database from other code, code which is coming some time in the future.
