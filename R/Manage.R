
# Connection management --------------------------------------------------------

#' MODB Connection References
#'
#' \code{modb_conn_ref} checks for and/or validates parameters that may be a
#' connection reference. Connections in MODB may be named on creation or must
#' be referenced by the unique ID that is returned by the
#' \link{\code{modb_connect}} method.
#'
#' @param conn_id Integer. The ID of the connection returned by modb_connect
#' @param conn_name String. The name provided when creating a connection.
#' @param conn_ref Expected to be either a conn_id or a conn_name
#' @param args list(...) Arguments passed to a calling function in the va scope.
#' @seealso \link{\code{modb_connect}}
#' @export
modb_conn_ref <- function(conn_id, conn_name, conn_ref, args = NULL) {
  if (length(args) != 0) {
    if ("conn_ref" %in% ls(args)) {
      conn_ref <- args$conn_ref
    }
    if ("conn_id" %in% ls(args)) {
      conn_id <- args$conn_id
    }
    if ("conn_name" %in% ls(args)) {
      conn_name <- args$conn_name
    }
  }

  if (!missing(conn_ref)) {
    if (checkmate::test_string(conn_ref)) {
      return(conn_ref)
    } else if (checkmate::test_int(conn_ref)) {
      return(as.integer(conn_ref))
    } else {
      stop("invalid connection ref provided (must be string(name) or int(id))")
    }
  } else if (!missing(conn_id)) {
    if (!checkmate::test_null(conn_id)) {
      checkmate::assert_int(conn_id)
      return(as.integer(conn_id))
    }
  } else if (!missing(conn_name)) {
    if (!checkmate::test_null(conn_name)) {
      checkmate::assert_string(conn_name)
      return(conn_name)
    }
  }

  stop("one of the arguments \"conn_ref\", \"conn_id\" or \"conn_name\" must be provided")
}


#' MODB Connections
#'
#' \code{modb_connect} creates a new database connection and attmepts to open a
#' connection to a database with the details provided.
#'
#' @param username String. Username to use when connecting to the database.
#' @param password String. Password to use when connecting to the database.
#' @param database String. The database to use.
#' @param host String. The hostname or IP address to connect to.
#'                     Either host & port or socket may be used
#' @param port Integer. The port number to connect on.
#' @param socket String. The socket path to connect to,
#'                       e.g. /var/run/mysqld/mysqld.sock
#' @param name String. A name for the connection which can be used to identify
#'                     the connection later on. Alternatively the id returned
#'                     can be used.
#' @return The id of the connection
#' @export
modb_connect <- function(username, password, database,
                         host = "localhost", port = 3306,
                         socket = NULL, conn_name = NULL) {
  checkmate::assert_string(username)
  checkmate::assert_string(password)
  checkmate::assert_string(database)

  if (checkmate::test_null(socket)) {
    checkmate::assert_string(host)
    checkmate::assert_int(port, lower = 1, upper = 65535)
    res <- .Call(
      c_modb_connectToHost,
      conn_name,
      host,
      as.integer(port),
      username,
      password,
      database
    )
  } else {
    res <- .Call(
      c_modb_connectToSocket, conn_name, socket, username, password, database
    )
  }

  if (res < 0) {
    stop("failed to create connection")
  }

  return(res)
}

#' MODB Connections
#'
#' modb_disconnect closes a database connection and cleans up the instance
#'
#' @param ... conn_id, conn_name or conn_ref required. See \link{modb_conn_ref}
#' @export
modb_disconnect <- function(...) {
  res <- .Call(c_modb_disconnect, modb_conn_ref(args = list(...)))
  return(invisible(res))
}

#' MODB Connections
#'
#' Fetches information on a connection such as the last query, insert id and
#' number of queries run
#'
#' @param ... conn_id, conn_name or conn_ref required. See \link{modb_conn_ref}
#' @return The details of the connection as a list.
#' @export
modb_connectionInfo <- function(stop_on_error = TRUE, ...) {
  conn_ref <- modb_conn_ref(args = list(...))
  utils::str(conn_ref)
  res <- .Call(c_modb_connectionInfo, conn_ref)

  if (length(res) == sum(is.na(res))) {
    if (stop_on_error) {
      stop("invalid connection reference")
    } else {
      warning("invalid connection reference")
      return(NA)
    }
  }

  return(res)
}

#' @describeIn modb_connectionInfo Determines if a connection with the name or
#'                                 id provided exists
#' @param ... conn_id, conn_name or conn_ref required. See \link{modb_conn_ref}
#' @return TRUE if the connection exists, FALSE if not.
#' @export
modb_connectionExists <- function(...) {
  res <- .Call(c_modb_connectionInfo, modb_conn_ref(args = list(...)))
  return(!(length(res) == sum(is.na(res))))
}

#' @describeIn modb_connectionInfo Finds a connection id via the name
#' @param conn_name String. The name provided when creating a connection.
#' @return The ID of the connection.
#' @export
modb_connectionId <- function(conn_name, stop_on_error = TRUE) {
  info <- modb_connectionInfo(stop_on_error, conn_ref = conn_name)
  if (length(info) == sum(is.na(info))) {
    return(NA)
  }
  return(info$id)
}
#' @describeIn modb_connectionInfo Finds a connection name via the id
#' @param conn_id Integer. The id of the connection, returned by modb_connect.
#' @return The name of the connection.
#' @export
modb_connectionName <- function(conn_id, stop_on_error = TRUE) {
  info <- modb_connectionInfo(stop_on_error, conn_ref = conn_id)
  if (length(info) == sum(is.na(info))) {
    return(NA)
  }
  return(info$name)
}


# Database management ----------------------------------------------------------

#' @export
modb_exists <- function(modb_name, ...) {
  conn_ref <- modb_conn_ref(args = list(...))
  res <- .Call(c_modb_exists, conn_ref, modb_name)
  return(res)
}
#' @export
modb_create <- function(modb_name, extended_meta = NULL, ...) {
  checkmate::assert_string(modb_name)

  # If a dataframe: data.frame(name = c(1, 2, 3), type = c('a', 'b', 'c'), nullable = c(1,0,1), stringsAsFactors=F)
  # extended_meta %>% rowwise() %>% transmute(r = list(c("A" = A, "B" = B, "C" = C))) %>% first()
  if (!checkmate::test_null(extended_meta)) {
    for (idx in 1:length(extended_meta)) {
      meta_col <- extended_meta[[idx]]

      if (!("name" %in% names(meta_col))) {
        stop("missing name in extended_meta definition")
      } else {
        checkmate::assert_string(meta_col$name)
      }

      if (!("type" %in% names(meta_col))) {
        stop("missing type in extended_meta definition")
      } else {
        type <- meta_col$type
        checkmate::assert_int(type, lower = 0, upper = TYPE_TIMESTAMP)
        checkmate::assert_true(type == 1 || type %% 2 == 0)
        meta_col$type <- as.integer(type)
      }

      if (!("nullable" %in% names(meta_col))) {
        meta_col$nullable = FALSE
      } else {
        checkmate::assert_logical(meta_col$nullable)
      }

      extended_meta[[idx]] <- meta_col
    }
  }

  conn_ref <- modb_conn_ref(args = list(...))
  res <- .Call(c_modb_create, conn_ref, modb_name, extended_meta)
  return(res)
}
#' @export
modb_destroy <- function(modb_name, ...) {
  conn_ref <- modb_conn_ref(args = list(...))
  res <- .Call(c_modb_destroy, conn_ref, modb_name)
  return(res)
}

#' @export
modb_use <- function(modb_name, override = FALSE, ...) {
  conn_ref <- modb_conn_ref(args = list(...))
  res <- .Call(c_modb_use, conn_ref, modb_name, as.logical(override))
  return(res)
}