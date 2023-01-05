# Cli-Launch-Scripts

These scripts launch the engine(s) passed to them in a command-line interface, at the moment [cutechess-cli](https://github.com/cutechess/cutechess), to run them in various usual and unusual situations. These test scenarios are useful for detecting bugs and limitations of engines through self-play.

Each script is designed with a specific goal in mind, as described here.

## Tourney-No-Inc-Test

There are several different time control schemes, one being Tournament time control, where each side gets `Y` amount of time to make `X` moves (written often as `X/Y`). Often, this kind of time control does not have increment, which means some engines poorly handle their time and step over the time margins. This time control is used in the [CCRL 40/15](https://www.computerchess.org.uk/ccrl/4040/) testings and various [CEGT](http://cegt.net/) matches.

I personally no longer use this time control scheme, since:

1. Due to how UCI handles this type of time control, the engine knows neither how much time it will get after completing the time control or how many moves the next time control will start at. This may make the time management of a UCI engine less optimal than that of an Xboard engine, which receieves this information through the `level` command.
2. This time control is less familiar to some developers, since increment-based time controls appear on online chess websites much more often. Engines, therefore, are more likely to be designed around playing in this setting.
3. Exiting recursive search at a precise point right before time expiring (which is generally optimal) is *hard*. Enough engines mess this up that I would rather not use it.

To test how frequency an engine runs out of time in this time control, the script runs games (100 by default) with the engine set by the `--engine` parameter running the protocol set by the `--proto` parameter in 4/6 seconds, which is expected to create about 20 instances per game which the engine could overstep the time control each game.

The output will be a cutechess-cli summary. The engine fails if a significant number of games are lost on time.
