# <p >slarchive 
###  SeedLink client for data stream archiving</p>

1. [Name](#)
1. [Synopsis](#synopsis)
1. [Description](#description)
1. [Options](#options)
1. [Seedlink Selectors](#seedlink-selectors)
1. [Archiving Data](#archiving-data)
1. [Stream List File](#stream-list-file)
1. [Caveats](#caveats)
1. [See Also](#see-also)
1. [Author](#author)

## <a id='synopsis'>Synopsis</a>

<pre >
slarchive [options] [host][:][port]
</pre>

## <a id='description'>Description</a>

<p ><b>slarchive</b> connects to a <i>SeedLink\fR server, requests data streams and writes received packets into directory/file structures. The precise layout of the directories and files is defined in a \fIformat</i> string.  Some preset file layouts are included: the SeisComP Data Structure (SDS), Buffer of Uniform Data structure (BUD) and a few others.  To write more than one archive simply specify multiple <i>format</i> definitions (or presets).</p>

## <a id='options'>Options</a>

<b>-V</b>

<p style="padding-left: 30px;">Print program version and exit.</p>

<b>-h</b>

<p style="padding-left: 30px;">Print program usage and exit.</p>

<b>-H</b>

<p style="padding-left: 30px;">Print program usage including details for the replacement flags used with the <i>-A</i> option and exit.</p>

<b>-v</b>

<p style="padding-left: 30px;">Be more verbose.  This flag can be used multiple times ("-v -v" or "-vv") for more verbosity.  One flag: report basic handshaking (link configuration) details and briefly report each packet received.  Two flags: report the details of the handshaking, each packet received and detailed connection diagnostics.</p>

<b>-p</b>

<p style="padding-left: 30px;">Print details of received Mini-SEED data records. This flag can be used multiple times ("-p -p" or "-pp") for more detail.  One flag: a single summary line for each data packet received.  Two flags: details of the Mini-SEED data records received, including information from fixed header and 100/1000/1001 blockettes.</p>

<b>-f </b><u>max</u>

<p style="padding-left: 30px;">The maximum number of files to keep open.  If not specified the maximum will be the default system process limit (for the given environment).  The maxium number of open archive data files will be maintained at 10 less than the absolute maximum leaving descriptors available for other tasks (e.g. writing state files).</p>

<b>-nd </b><u>delay</u>

<p style="padding-left: 30px;">The network reconnect delay (in seconds) for the connection to the SeedLink server.  If the connection breaks for any reason this will govern how soon a reconnection should be attempted. The default value is 30 seconds.</p>

<b>-nt </b><u>timeout</u>

<p style="padding-left: 30px;">The network timeout (in seconds) for the connection to the SeedLink server.  If no data [or keep alive packets?] are received in this time the connection is closed and re-established (after the reconnect delay has expired).  The default value is 600 seconds. A value of 0 disables the timeout.</p>

<b>-k </b><u>keepalive</u>  (requires SeedLink >= 3)

<p style="padding-left: 30px;">Keepalive packet interval (in seconds) at which keepalive (heartbeat) packets are sent to the server.  Keepalive packets are only sent if nothing is received within the interval.</p>

<b>-x </b><u>statefile</u>[:<u>interval</u>]

<p style="padding-left: 30px;">During client shutdown the last received sequence numbers and time stamps (start times) for each data stream will be saved in this file. If this file exists upon startup the information will be used to resume the data streams from the point at which they were stopped.  In this way the client can be stopped and started without data loss, assuming the data are still available on the server.  If <u>interval</u> is specified the state will be saved every <u>interval</u> packets that are received.  Otherwise the state will be saved only on normal program termination.</p>

<b>-i </b><u>timeout</u>

<p style="padding-left: 30px;">Timeout for closing idle data stream files in seconds.  The idle time of data streams is only checked when a packet has arrived so if no packets are arriving no idle stream files will be closed.  There should be no reason to change this parameter except for unusual cases where the process is running against an open file number limit. Default is 300 seconds.</p>

<b>-d</b>

<p style="padding-left: 30px;">Configure the connection in "dial-up" mode.  The remote server will close the connection when it has sent all of the data in it's buffers for the selected data streams.  This is opposed to the normal behavior of waiting indefinately for data.</p>

<b>-b</b>

<p style="padding-left: 30px;">Configure the connection in "batch" mode.  Negotiation with the remote server is made faster by minimizing acknowledgement checks.</p>

<b>-Fi[:overlap]</b>

<p style="padding-left: 30px;">Future check initially.  Check the last Mini-SEED data record in an existing archive file and do not write new data to that file if it is older within a certain overlap.  The default overlap limit is 2 seconds; the overlap can be specified by appending a colon and the desired overlap limit in seconds to the option.  If the overlap is exceeded an error message will be logged once for each time the file is opened.  This option only makes sense for archive formats where each unique data stream is written to a unique file (e.g. SDS format). If a data stream is closed due to timeout (see option -i) the initial future check will be preformed again if the file is re-opened.</p>

<b>-Fc[:overlap]</b>

<p style="padding-left: 30px;">Future check continuously.  Only archive Mini-SEED data records if the first sample of the record is newer, within a certain overlap, than the last sample of the previous record for a given archive file.  The default overlap limit is 2 seconds; the overlap can be specified by appending a colon and the desired overlap limit in seconds to the option.  If the overlap is exceeded an error message will be logged once until either a non-overlapping packet is received or a new archive file is used.  This option only makes sense for archive formats where each unique data stream is written to a unique file (e.g. SDS format).</p>

<b>-s </b><u>selectors</u>

<p style="padding-left: 30px;">This defines default selectors.  If no multi-station data streams are configured these selectors will be used for uni/all-station mode. Otherwise these selectors will be used when no selectors are specified for a given stream using the '-S' or '-l' options.</p>

<b>-l </b><u>listfile</u>

<p style="padding-left: 30px;">A list of streams will be read from the given file.  This option implies multi-station mode.  The format of the stream list file is given below in the section <u>Stream list file</u>.</p>

<b>-S </b><u>stream[:selectors],...</u>  (requires SeedLink >= 2.5)

<p style="padding-left: 30px;">The connection will be configured in multi-station mode with optional SeedLink selectors for each station, see examples below.  <u>stream</u> should be provided in NET_STA format.  If supported by the server the '*' or '?' wildcards may be used in the NET_STA designation.  If no selectors are provided for a given stream, the default selectors, if defined, will be used.</p>

<b>-tw </b><u>start:[end]</u>  (requires SeedLink >= 3)

<p style="padding-left: 30px;">Specifies a time window applied, by the server, to data streams.  The format for both times is year,month,day,hour,min,sec; for example: "2002,08,05,14,00:2002,08,05,14,15,00".  The end time is optional but the colon must be present.  If no end time is specified the server will send data indefinately.  This option will override any saved state information.</p>

<p style="padding-left: 30px;">Warning: time windowing might be disabled on the remote server.</p>

<b>-A </b><u>format</u>

<p style="padding-left: 30px;">If specified, all packets (Mini-SEED records) received will be appended to a directory/file structure defined by <b>format</b>.  All directories implied in the <b>format</b> string will be created if necessary.  The option may be used multiple times to write received packets to multiple archives.  See the section <u>Archiving data</u>.</p>

<b>-SDS </b><u>SDSdir</u>

<p style="padding-left: 30px;">If specified, all packets (Mini-SEED records) received will be saved into a Simple Data Structure (SDS) dir/file structure starting at the specified directory.  This directory and all subdirectories will be created if necessary.  This option is a preset version of the '-A' option.  The SDS dir/file structure is:</p>
<pre style="padding-left: 30px;">
<SDSdir>/<YEAR>/<NET>/<STA>/<CHAN.TYPE>/NET.STA.LOC.CHAN.TYPE.YEAR.DAY
</pre>

<b>-BUD </b><u>BUDdir</u>

<p style="padding-left: 30px;">If specified, all waveform data packets (Mini-SEED data records) received will be saved into a Buffer of Uniform Data (BUD) dir/file structure starting at the specified directory.  This directory and all subdirectories will be created if necessary.  This option is a preset version of the '-A' option.  The BUD dir/file structure is:</p>
<pre style="padding-left: 30px;">
<BUDdir>/<NET>/<STA>/STA.NET.LOC.CHAN.YEAR.DAY
</pre>

<b>-CSS </b><u>CSSdir</u>

<p style="padding-left: 30px;">If specified, all packets (Mini-SEED data records) received will be saved into a CSS-like data file structure starting at the specified directory.  This directory and all subdirectories will be created if necessary.  This option is a preset version of the '-A' option.  The CSS dir/file structure is:</p>
<pre style="padding-left: 30px;">
<CSSdir>/<YEAR>/<DAY>/STA.CHAN.YEAR:DAY:HOUR:MIN:SEC
</pre>
<p >where the HOUR, MIN & SEC fields are non-defining substitutions.</p>

<b>-CHAN </b><u>dir</u>

<p style="padding-left: 30px;">If specified, all packets (Mini-SEED data records) received will be saved into channel files in the specified directory.  This directory will be created if necessary.  This option is a preset version of the '-A' option.  The CHAN dir/file structure is:</p>
<pre style="padding-left: 30px;">
<dir>/NET.STA.LOC.CHAN
</pre>

<b>-QCHAN </b><u>dir</u>

<p style="padding-left: 30px;">If specified, all packets (Mini-SEED data records) received will be saved into channel files including the data quality identifier in the specified directory.  This directory will be created if necessary. This option is a preset version of the '-A' option.  The QCHAN dir/file structure is:</p>
<pre style="padding-left: 30px;">
<dir>/NET.STA.LOC.CHAN.QUALITY
</pre>

<b>-CDAY </b><u>dir</u>

<p style="padding-left: 30px;">If specified, all packets (Mini-SEED data records) received will be saved into channel-day files in the specified directory.  This directory will be created if necessary.  This option is a preset version of the '-A' option.  The CDAY dir/file structure is:</p>
<pre style="padding-left: 30px;">
<dir>/NET.STA.LOC.CHAN.YEAR.DAY:HOUR:MIN:SEC
</pre>
<p >where the HOUR, MIN & SEC fields are non-defining substitutions.</p>

<b></b><u>[host][:][port]</u>

<p style="padding-left: 30px;">A required argument, specifies the address of the SeedLink server in host:port format.  Either the host, port or both can be omitted.  If host is omitted then localhost is assumed, i.e.  ':18000' implies 'localhost:18000'.  If the port is omitted then 18000 is assumed, i.e.  'localhost' implies 'localhost:18000'.  If only ':' is specified 'localhost:18000' is assumed.</p>

## <a id='seedlink-selectors'>Seedlink Selectors</a>

<p ><u>Notes regarding selectors from a SeedLink configuration file</u></p>

<p >The "selectors" parameter is used to request specific packets, in effect limiting the default action of sending all data. A packet is sent to the client if it matches any positive selector (without leading "!") and doesn't match any negative selectors (with "!").  The general format of selectors is LLSSS.T, where LL is location, SSS is channel, and T is type (one of DECOTL for Data, Event, Calibration, Blockette, Timing, and Log records).  "LL", ".T", and "LLSSS." can be omitted, meaning "any".  It is also possible to use "?" in place of L and S.</p>

<pre >

Some examples:
BH?            - BHZ, BHN, BHE (all record types)
00BH?.D        - BHZ, BHN, BHE with location code '00' (data records)
BH? !E         - BHZ, BHN, BHE (excluding detection records)
BH? E          - BHZ, BHN, BHE & detection records of all channels
!LCQ !LEP      - exclude LCQ and LEP channels
!L !T          - exclude log and timing records
</pre>

## <a id='archiving-data'>Archiving Data</a>

<p >Using the '-A <b>format</b>' option received data can be saved in a custom directory and file structure.  The archive <b>format</b> argument is expanded for each packet processed using the following flags:</p>

<pre >
  <b>n</b> : network code, white space removed
  <b>s</b> : station code, white space removed
  <b>l</b> : location code, white space removed
  <b>c</b> : channel code, white space removed
  <b>Y</b> : year, 4 digits
  <b>y</b> : year, 2 digits zero padded
  <b>j</b> : day of year, 3 digits zero padded
  <b>H</b> : hour, 2 digits zero padded
  <b>M</b> : minute, 2 digits zero padded
  <b>S</b> : second, 2 digits zero padded
  <b>F</b> : fractional seconds, 4 digits zero padded
  <b>q</b> : record quality indicator (D,R,Q,M), single character
  <b>L</b> : data record length in bytes
  <b>r</b> : sample rate (Hz) as a rounded integer
  <b>R</b> : sample rate (Hz) as a float with 6 digit precision
  <b>%</b> : the percent (%) character
  <b>#</b> : the number (#) character
  <b>t</b> : single character type code:
         D - waveform data packet
         E - detection packet
         C - calibration packet
         T - timing packet
         L - log packet
         O - opaque data packet
         U - unknown/general packet
         I - INFO packet
         ? - unidentifiable packet
</pre>

<p >The flags are prefaced with either the <b>%</b> or <b>#</b> modifier. The <b>%</b> modifier indicates a defining flag while the <b>#</b> indicates a non-defining flag.  All received packets with the same set of defining flags will be saved to the same file. Non-defining flags will be expanded using the values in the first packet received for the resulting file name.</p>

<p >Time flags are based on the start time of the given packet.</p>

<p >For example, the format string:</p>

<p ><b>/archive/%n/%s/%n.%s.%l.%c.%Y.%j</b></p>

<p >would be expanded to day length files named something like:</p>

<p ><b>/archive/NL/HGN/NL.HGN..BHE.2003.055</b></p>

<p >Using non-defining flags the format string:</p>

<p ><b>/data/%n.%s.%Y.%j.%H:#M:#S.miniseed</b></p>

<p >would be expanded to:</p>

<p ><b>/data/NL.HGN.2003.044.14:17:54.miniseed</b></p>

<p >resulting in hour length files because the minute and second are specified with the non-defining modifier.  The minute and second fields are from the first packet in the file.</p>

## <a id='stream-list-file'>Stream List File</a>

<p >The stream list file used with the '-l' option is expected to define a data stream on each line.  The format of each line is:</p>

<pre >
<net> <station> [selectors]
</pre>

<p >The selectors are optional.  If default selectors are also specified (with the '-s' option), they they will be used when no selectors are specified for a given stream.  An example file follows:</p>

<pre >
----  Begin example file -----
# Comment lines begin with a '#' or '*'
# Example stream list file for use with the -l argument of slclient or
# with the sl_read_streamlist() libslink function.
GE ISP  BH?.D
NL HGN
MN AQU  BH? HH?
II *    BHZ
----  End example file -----
</pre>

## <a id='caveats'>Caveats</a>

<p >The future data checking options (-Fi and -Fc) only control the writing of waveform data to archive files.  Any duplicates of other packet types sent by the server will be written to their associated archive file.</p>

<p >The future data checking options (-Fi and -Fc) are only consistent for unique data streams written to a single archive file; in other words, the checks do not span across different archive files.  As an example, the SDS format creates "day files" which rotate at midnight.  The future checks will not function correctly if there is a time jump starting with the first packet in a new day file at midnight.  The chance of this occurring is very, very low, but the behavior should be noted nonetheless.</p>

<p >The initial future check (-Fi) triggers slarchive to perform a future check whenever a file is opened.  An existing file can be re-opened if it has been closed due to an idle stream timeout.</p>

<p >In general it is not a good idea to use non-defining modifier flags (those starting with '#') in directory names.  Doing this with data streams that are closed due to timeout and re-opened or slarchive restarts will result in empty directories being created.  This is such a fringe case that it will not be addressed any time soon.</p>

## <a id='see-also'>See Also</a>

<p ><b>slinktool</b>(1)</p>

## <a id='author'>Author</a>

<pre >
Chad Trabant
IRIS Data Management Center
</pre>


(man page 2010/03/10)
