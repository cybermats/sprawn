# Sprawn - editor

Sprawn is a text editor supporting very large files across all modern platforms.

## Architecture

The system consists of a few different modules. The frontend is responsible for showing the text interface and handles conversions of file encodings etc. The backend is responsible for reading input and handling i/o. The middleware controller is going to be the location for plugins, such as font highlighting, search, and general plugin infrastructure that we will add in the future.

### Frontend
The frontend is modular to support multiple ways of rendering the editor. The initial version supports a cross platform window system, displaying unicode text and can display most modern languages (latin, chinese and arabic for name a few) and emojis.

The frontend keeps a local small cache of the currently and recently displayed lines. It renders all text layout internally (no widget toolkit).

The frontend receives the text encoded as UTF8 Unicode from the backend. The frontend is responsbile for making modifications to the text in order to display it, such as removing \r from Windows or Mac based files.

### Backend
The backend can read from many types of sources, streaming from URLs or random access such as files. The files is opened using memory mapped files in order to maintain a small memory footprint.
The backend handles editing using a Piece Table. That way we can open very large files and still support swift editing.
The backend supports many types of character encoding (UTF8, UTF16, UTF32, ASCII, ISO 8859-1 to name the modern ones). It maintains the original character encoding internally but exposes a common encoding to the frontend, such as unicode and UTF8.

### Middleware Controller
The middleware is responsible for the interaction between the frontend and the backend. Initially it wont do anything.

