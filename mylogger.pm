#!/usr/bin/perl

package mylogger;

use DBI;
use POSIX;
use vars qw/$dbh $sth/;

sub log($$$) {
    my $sender = shift;
    my $group = shift;
    my $message = shift;

    $dbh ||= DBI->connect("DBI:mysql:database=weblogs", "logger", "", { RaiseError => 0 });
    warn "DBI->connect failed." unless($dbh);
    if($dbh) {
        $sth ||= $dbh->prepare(q{INSERT INTO logs (host, domain, timestamp, data) VALUES(?,?,?,?)});
    }

    my ($user, $host) = ($sender =~ /#([^#]+)#([^#]+)/);
    my ($time) = ($message =~ /\[([^\]]+)\]/);
    $time ||= stftime("%d/%b/%Y:%H:%M:%S %z", localtime(time));
    $sth && $sth->execute($host, $group, $time, $message);
    print STDERR "Insert($host, $group, $time, $message);\n";
}
1;
