# RMODB (R Meta-Object Data Base)

<!-- badges: start -->
<!-- badges: end -->

RMODB implements a metadata-object database for storing R objects in MySQL. R objects are serialized into a binary memory buffer which is then written into a MySQL database along with metadata such as creation timestamp, update timestamp, owner, title etc. The metadata fields are customisable on creation of the database and the fields are cleartext and queryable allowing objects to be found and returned by filtering on any metadata column or combination of columns. On returning an object, it is de-serialised using the reverse of the serialize to binary memory process.

## Versions
0.0.1 - Current development

## Installation

You can install the current version of modb from [Github](https://github.com/avsdev-uk/rmodb) using the devtools package:

``` r
devtools::install_github('avsdev-uk/rmodb')
```

You can install the a released version of modb from [CRAN](https://CRAN.R-project.org) with:

``` r
install.packages("rmodb")
```

## Example

All methods within RMODB are documented in R using Roxygen.

This is a basic example which shows you how to solve a common problem:

``` r
library(rmodb)
## basic example code
```
