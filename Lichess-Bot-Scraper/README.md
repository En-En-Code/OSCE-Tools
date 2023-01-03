# Lichess-Bot-Scraper

[Lichess](https://lichess.org) is a free and open source chess server which has a small community of engine developers. By using a [Lichess Bot](https://lichess.org/blog/WvDNticAAMu_mHKP/welcome-lichess-bots), chess engines can play games on Lichess.

Many bot developers make their projects open source and include links to their repository in the account description. Through the [Lichess API](https://lichess.org/api), a program could obtain a list of links to these open repositories.

## Usage

The code can be run with [Node.js](https://nodejs.org) in command line: `node Lichess-Bot-Scraper.js`.

## Implementation Details

There are two candidate APIs a program could call:

1. GET Team Members `/api/team/{teamId}/users`, where `teamId` is the ID of a team known to have many bots.
2. GET Online Bots `/api/bot/online`, which obtains a list of online bot accounts.

I choose the latter, since:

1. Joining a team is optional, and therefore would always miss some bots.
2. There are several bot-focused teams one could pick from, and calling the Lichess API multiple times to account for all of these is both more complicated and likely to be highly redundant.
3. A bot which is online regularly is more likely to be functional and/or worked on.

The structure of the program should be simple:

1. Use the Lichess API to GET online bots, throwing an error if something goes wrong.
2. Parse the `bio` and `links` properties of each account object for URLs to known repository hosting websites, removing some common links known not to be engines themselves and add the links to a global tracking object.
4. Return the object, indicating success.

There are a couple hiccups to this linear plan. First, `/api/bot/online` returns [newline-delimited JSON](http://ndjson.org/), which means the input must be split at newlines before being parsed into JSON, although any given stream chunk may contain no or multiple newlines. Second, and the bigger issue, is that the Lichess API throttles output to 50 accounts a second. Because of this, it is best to perform the second step on JSON objects the first has already completed while the first continues collecting data.

## TODO

Improve the regular expressions used to match repositories, through better username and repository regexes where I took guesses and/or adding more code-hosting sites. Currently only GitHub, GitLab, Bitbucket, Codeberg, and SourceForge are supported.

In the future, the returned object could be used as input to another program, such as one which compares the found links against a list of known repositories.
