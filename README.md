# spreadlogd #

Spreadlogd is a simple tool to log messages received via the [Spread Group Communication system](http://www.spread.org/).  It is primarily used in conjuction with [mod_log_spread](http://www.backhand.org/mod_log_spread/) to journal a centralized log file for a cluster of Apache servers.

Due to a loadable module system, it can be enhanced to passively analyze the messages that it witnesses.  This can lead to the development of interesting and innovative tools.

## License ## 
This software is made available under the Artistic license. Please [read the license](Artistic.txt) before using this software.

## Additional Info ##
This software works great for me.  Other than the options seen with -h
there are a few tid bits of knowledge to know.

Edit the makefile and uncomment/comment the right parts in the architecture
dependant sections.

If you kill -HUP or kill the spreadlogd process, it will not actually
process the signal until after it has received its next message from
Spread.  You can move you log files to new names and then kill -HUP 
and it will reopen the log files.  This is useful for seamless log rotation
without losing any messages.

Spread is really cool.  It is a poweful group communication toolkit 
developed at the Center for Networking and Distributed Systems at the
Johns Hopkins University.  http://www.spread.org/ and
http://www.cnds.jhu.edu/, respectively.


