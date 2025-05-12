# faraway

![](img/faraway-large.png)

`faraway` scans a part of a directory hierarchy and
stores its structure inside an index file.
The index file can then be consulted even if the directory hierarchy itself
is *far away* (e.g. external hard drive, network share).

The key difference between `faraway` and tools like
`updatedb`/`locate`
is that `faraway` uses a tree structure to store the index.
That means that listing the contents of a specific directory
inside a `faraway` index is really fast.

In other words, if `updatedb` is a "reverse index"
that answers the question "where is XYZ?",
then `faraway` is a "forward index" that answers the question
"what's inside XYZ?"

## Compiling

Run `make` from the root of the repository.
This will compile `build/faraway` using GCC.

Static and dynamic builds are also available on the
[releases page](https://github.com/maybeetree/faraway/releases).

## Using

Create an index:

```
./build/faraway /path/to/index/file.fa scan /path/to/target/dir
```

Look inside an index:

```
./build/faraway /path/to/index/file.fa ls /path/inside/index
```

## Further work

`faraway` is currently a very simple program,
but I have more plans for it.

- [ ] Handling truly massive directory hierarchies
    - [x] Read index without loading the entire file into memory
    - [ ] Write to disk without keeping entire index in memory while scanning
    - [ ] Make tests
- [ ] Interoperability
    - [ ] Import index from `locate` database
    - [ ] Export index to `locate` databsase
    - [ ] Import index from `mtree` format
    - [ ] Export index to `mtree` format
- [ ] Attributes
    - [ ] Attrs
    - [ ] Xattrs
    - [ ] Symlink target
    - [ ] Hardlink info
    - [ ] Reflink info?
    - [ ] btrfs-specific (e.g. subvolume)
- [ ] General
    - [ ] valgrind
    - [ ] unit tests
- [ ] Monitor FS changes with inotifyd
- [ ] Custom root path

## License

`faraway` is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, version 3 of the License only.

`faraway` is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
`faraway`. If not, see <https://www.gnu.org/licenses/>. 

---

Copyright (c) 2025, maybetree.

