# RMetaObjDB

<!-- badges: start -->
<!-- badges: end -->

RMetaObjDB implements a metadata-object database for storing R objects in MySQL. R objects are serialized into a binary memory buffer which is then written into a MySQL database along with metadata such as creation timestamp, update timestamp, owner, title etc. The metadata fields are customisable on creation of the database. the metadata fields are cleartext and queryable allowing objects to be found and returned. On returning an object, it is de-serialised using the reverseof the serialize to binary memory process.

## Installation

You can install the released version of RMetaObjDB from [CRAN](https://CRAN.R-project.org) with:

``` r
install.packages("RMetaObjDB")
```

## Example

This is a basic example which shows you how to solve a common problem:

``` r
library(RMetaObjDB)
## basic example code
```
