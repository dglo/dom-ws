#
# open a socket connection...
#

#
# defaults
#
host <- "klauspc.dhcp.lbl.gov"
port <- 2001

#
# this should only be done once...
#
#con <- socketConnection(host=host, port=port)

#
# send an arbitrary command...
#
cmd <- function(text) {
  #
  # make sure we're terminated by a '\r'
  #
  if (substr(text, nchar(text), nchar(text))!='\r') {
    text <- paste(text, '\r', sep="");
  }

  #
  # clear old data
  #
  while (length(readLines(con))!=0) {}

  #
  # write text...
  #
  writeLines(text, con);

  #
  # wait for a bit...
  #
  Sys.sleep(0.5);

  #
  # read back data...
  #
  readLines(con)
}

#
# return nloop columns of atwd data...
#
atwdLoop <- function(con, chip='A', channel=0, nloop=100) {
  ret <- NULL
  for (i in 1:nloop) {
    print(paste("loop:", i, "of", nloop));
    cmd('atwdrs atwdatr')
    ret <- cbind(ret, atwdReadout(con, chip, channel))
  }
  cmd('atwdrs')

  ret
}

atwdReadout <- function(con, chip='A', channel=0) {
  if (chip=='A')      { base <- "$90084000" }
  else if (chip=='B') { base <- "$90085000" }
  else stop("invalid chip (must be A or B)");

  offset <- channel * 128*4

  writeLines(paste(base, offset, '+', '128 od\r', sep=" "), con);

  #
  # wait 100ms...
  #
  Sys.sleep(0.5);

  lns <- readLines(con, n=34, ok=FALSE)

  #
  # parse the data...
  #
  odToInteger(lns[2:33])
}

#
# convert od output to integer array...
#
# returns vector of length 128 if everything
# is ok or length 0 if not...
#
odToInteger <- function(lines) {
  #
  # get data as a matrix...
  #
  ret <- matrix(as.integer(sapply(lines, odLineToInteger)), nrow=5, ncol=32)

  #
  # verify that the addresses are correct
  #
  addr <- as.integer(seq(ret[1, 1], ret[1, 1]+4*4*31, by=4*4))
  if (!identical(addr, ret[1, ])) {
    print(matrix(c(addr, ret[1, ]), ncol=2))
    stop("invalid address in od dump, data error?")
  }

  #
  # return just the raw data...
  #
  as.numeric(ret[2:5, ])
}

#
# get the short portion of the 8 entry hex number
#
hexToInteger <- function(hex) {
  as.integer(paste("0x", substr(hex, 5, 8), sep=""))
}

#
# convert a line from od to integers, we only use
# the lower 16 bits...
#
odLineToInteger <- function(lines) {
  as.integer(lapply(strsplit(lines, " "), hexToInteger)[[1]])
}

#
# create baseline waveform (pedestal) vector...
#
mkPedestal <- function(atwd) { apply(atwd, 1, mean) }

#
# subtract pedestal from waveform matrix...
#
subPedestal <- function(atwd, pedestal) {
  atwd - matrix(rep(pedestal, ncol(atwd)), nrow=128)
}

#
# initial setup...
#
print(cmd('s" mycmds.fs" find if interpret endif'))
print(cmd('ledmux'))
print(cmd('fpga-versions'))

