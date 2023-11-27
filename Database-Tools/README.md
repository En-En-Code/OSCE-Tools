# Database-Tools

A database is far better at being interacted with by other code than a spreadsheet. Therefore, I am migrating from a spreadsheet to a [PostgreSQL](https://www.postgresql.org/) database, to make features such as automatically checking repositories for updates significantly easier.

## Usage

After installing PostgreSQL, run:
* `sudo -i -u postgres` to change to the `postgres` user,
* `createuser --interactive` to make a user with less power than `postgres`, and
* `createdb engine_db -O <username>` to create the database owned by the user.
  * Additionally, you may wish to modify `/var/lib/postgres/data/pg_hba.conf` to restrict database access.
* After leaving the `postgres` user, do `systemctl start postgresql` to start the local server,
  * (or `systemctl enable --now postgresql.service`) to have postgresql launch on every startup,)
* `psql -d engine_db` to have a direct interface to the database.

More likely, however, you will want a way to call the database from other code, code which is coming some time in the future.
