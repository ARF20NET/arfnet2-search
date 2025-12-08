# arfnet2-search

ARFNET Fast file index and search

## Description

 - C webapp
 - libmicrohttpd

## Features

 - All cached indexed in memory
 - Searchbox
 - Periodic reindexing and inotify
 - Searching
    - Advanced name substring, exact, regex
    - Sorting
    - Filtering

## Building

Depends on libmicrohttpd, libmagic

```
make
```

## TODO

 - [ ] Regex query
 - [ ] inotify

## Bugs

 - [ ] Query type not saved on submit
 - [ ] Long output gets cut after ~300 results

