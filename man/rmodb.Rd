% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/RMODB.R
\docType{package}
\name{rmodb}
\alias{rmodb}
\title{RMODB: Metadata-Object pair database}
\description{
RMODB implements a metadata-object database for storing R objects in
MySQL. R objects are serialized into a binary memory buffer which is then
written into a MySQL database along with metadata such as creation
timestamp, update timestamp, owner, title etc. The metadata fields are
customisable on creation of the database. the metadata fields are cleartext
and queryable allowing objects to be found and returned. On returning an
object, it is de-serialised using the reverseof the serialize to binary
memory process.
}
