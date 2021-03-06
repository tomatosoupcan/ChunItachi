# ChunItachi

ChunItachi is an injectable dll for Chunithm Amazon/AmazonPlus/Crystal which enables [KamaItachi](https://kamaitachi.xyz) uploads as you play

## Installation & Usage

Make sure you have [Microsoft VC++ Runtimes](https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads) installed.

Make sure you have your segatools.ini configured correctly and have an options folder, I think that still crashes things, maybe I'll fix it someday.

Modify your start.bat to mirror:

```bash
inject -d -k chunihook.dll -k ChunItachi.dll chuniApp.exe
```

Place ChunItachi.dll, zlib1.dll, libcurl.dll, and ChunItachi.ini  in your bin directory.

Config file info, everything else should mostly remain untouched:
- extid, set this to your extid for the account you want to register scores with, you can find this in your db (sega_card table in Aqua, aime_player in Minime). If you can't find it, or are unable to access your db, set this to some junk value like 123, then play a single song and check the debug logs for the mismatch info.
- game, set this to any of amazon, amazonplus, or crystal
- username, set this to your kamai username
- password, set this to your kamai password
- failOverLamp, set this to false if you would prefer your lamps to take precedent over the FAILED lamp, set this to true if you want FAILED to take precedent

You need an option folder defined in segatools.ini

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License
[MIT](https://choosealicense.com/licenses/mit/)
