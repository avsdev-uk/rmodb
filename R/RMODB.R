#' RMODB: Metadata-Object pair database
#'
#' RMODB implements a metadata-object database for storing R objects in
#' MySQL. R objects are serialized into a binary memory buffer which is then
#' written into a MySQL database along with metadata such as creation
#' timestamp, update timestamp, owner, title etc. The metadata fields are
#' customisable on creation of the database. the metadata fields are cleartext
#' and queryable allowing objects to be found and returned. On returning an
#' object, it is de-serialised using the reverseof the serialize to binary
#' memory process.
#'
#' @docType package
#' @name rmodb
#' @useDynLib rmodb, .registration = TRUE, .fixes = "c_"
NULL
#> NULL

.onUnload <- function(nspath) {
  # clear all connections
}