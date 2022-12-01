# Contributing

This document gives some information for Circle users, who want to help developing this project. Circle is an open source project and like any project of this kind, it lives from its users. So first thank you for the effort you spent for Circle! Contributions are welcome.

## Issues

If you encounter a problem or want a new feature in Circle, you can create a new issue [here](https://github.com/rsta2/circle/issues). Before you do so, please check, if the same issue has been created by another user yet. You can use the search function to accomplish this. Some basic known issues have been reported [here](doc/issues.txt). Please check them too.

Please take a moment to think about, if your request is not related more to the [circle-stdlib project](https://github.com/smuehlst/circle-stdlib), if you are using it together with Circle. If so, please create your issue there.

## Pull Requests

If you want to contribute corrections, bug fixes or new features, you can create a pull request. Please read the [GitHub help](https://docs.github.com/en/free-pro-team@latest/github/collaborating-with-issues-and-pull-requests/creating-a-pull-request) on how this works.

Some notes on this:

* Circle is developed using this [Git branching model](http://nvie.com/posts/a-successful-git-branching-model). Even that this model was conceived in 2010 for web apps it still proves to fit well for Circle development. Unlike today's web apps Circle is not delivered continuously. Instead releases (also called "steps") are provided from time to time on the master branch. In this model normal development takes place on the "develop" branch. So if you want to contribute, please branch off from the "develop" branch, and specify "develop" as target branch for the pull request, because contributions will always be merged here.

* Be sure your your Git user info (name, email) is properly set, when creating the commit(s) for your pull request.

* Please have a look at the coding style, used in Circle, before applying modifications. It is very appreciated, if you adapt the same style (use tabs, tabs are 8 characters wide, maximum line length is 100 characters, braces "{}" should be on a separate code line). Because the source code is an important reference in Circle, a unique style is vital for a good readability.

* If you plan greater modifications, please contact [me](mailto:rsta2@o2online.de) before proceeding. Not each update of this kind can be accepted. Circle is a light-weighted system of tightly coupled C++ classes. It's probably not that easy to consider all requirements, taken into account, when implementing new features of greater size.

* Important references of Circle are the [class header files](include/circle). Please try to make them easy to read. Make it simple, if possible! Public new class interfaces should be documented for [Doxygen](https://www.doxygen.nl).

* Before committing your changes, please test them carefully! If you have multiple Raspberry Pi models, please use them for the tests. Normally Circle features must work on all models and in 32-bit and 64-bit mode. It is clear, that not everybody has a collection of all different Raspberry Pis, but use the models, that you have.

* You must be allowed to contribute the source code, you want to provide. Of course, if you have written it by yourself, that is not a problem. But if you are using third-party source code, the situation my be different.

## Finally

Please do not expect to much, do not try to change "everything". Circle has grown over some years now and the concepts behind have been proven to be suitable. Of course, that does not mean, that everything is perfect and improvements are always possible, but have to be weighted too, if they are good for the project.

Thanks again for your work!
