# Me TV – it's TV for me computer #

Me TV is a digital television viewer for GNOME.

Home Page: http://launchpad.net/me-tv

## Reporting Issues

Launchpad remains the project's home page and location, GitHub is being used really just for code
development purposes – and to gain access to the variety of CI resources, and the like, which work with
GitHub but not with Launchpad. So, if you have questions, bug reports, issues, etc. please go to the
[Home Page](http://launchpad.net/me-tv). Repository forking and pull requests should all happen on GitHub.

## Nota Bene

This repository is not based on the mainline Bazaar branch to be found on Launchpad. That branch is numbered
1.4 but is based on the 2.0 work rather than the 1.3 branch. The 2.0 architecture was a full
client–server architecture the idea being to make Me TV a complete entertainment centre. It is not clear
that that is the right direction for this project. So instead, this repository is based on the 1.3 branch
with the intention to create a very lightweight digital television viewer.

The current codebase is GTK+2 based and is a mix of C and C++. The work here is to transform this into a
C++14 codebase for GTK+3.

The master branch is currently the GTK+2 version of the code and is used for transforming as much code as
possible from C and old C++ to C++14. Compiling this branch should always lead to a system that works.  The
gtk3 branch pulls all the appropriate changes from master and includes all the changes necessary to run with
GTK+3. Once the gtk3 branch compiles and runs, which it doesn't as yet, it will be merged into master.

## Building

The original Autotools system remains and is kept up to date, but a [SCons](http://www.scons.org) build has
been added and this is what is currently used for development.
