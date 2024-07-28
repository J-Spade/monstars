# swackhammer

**swackhammer** is a Django app that provides static resources and templates for Monstars' webapps to use. The look and feel of the site is based on [the classic 1996 Space Jam website](https://www.spacejam.com/1996/jam.htm).

The `swackhammer` webapp includes URLs for user signin and signout, as well as a main index from which installed monstars' webapps can be reached.


## Installation

TODO


## Player Integration

To plug into the `swackhammer` framework, ensure that `swackhammer` is added as a dependency of the Monstar's Python package.

In the `apps.py` for the webapp, ensure the `AppConfig` class contains the following members:
 - `monstars_bio`: a short description of what the monstar player generally does
 - `nerdluck_img`: a static image resource depicting the "un-talented" Nerdluck version of the Monstars player
 - `monstar_img`: a static image resource depicting the "talented" Monstars player

Like any Django app, Monstars' webapps must be added to the `INSTALLED_APPS` in the main Django site's `settings.py`. URLs are managed by the `swackhammer` app, so no other adjustments should be needed in the site configuration.