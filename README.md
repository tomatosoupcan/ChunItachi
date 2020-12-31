# ChunItachi

ChunItachi is an injectable dll for Chunithm Amazon/AmazonPlus/Crystal which enables KamaItachi uploads as you play

## Installation & Usage

Make sure you have [Microsoft VC++ Runtimes](https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads) installed.

Modify your start.bat to mirror:

```bash
inject -d -k chunihook.dll -k ChunItachi.dll chuniApp.exe
```

Place ChunItachi.dll, zlib1.dll, libcurl.dll, and ChunItachi.ini  in your bin directory.

You need an option folder defined in segatools.ini

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License
[MIT](https://choosealicense.com/licenses/mit/)
