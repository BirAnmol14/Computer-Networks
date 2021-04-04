set ns [new Simulator]
#simulation file
set nf [open out1.nam w]
$ns namtrace-all $nf
#trace file
set tf [open outall1.tr w]
$ns trace-all $tf
#cwnd file
set cf [open cwnd1.tr w]
#set color
$ns color 1 Blue

#designing architecture
set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]

$ns duplex-link $n0 $n1 2Mb 100ms DropTail
$ns duplex-link $n1 $n2 2Mb 100ms DropTail

#finish procedure
proc finish { } {
	global ns nf tf cf
	close $nf
	close $tf 
	close $cf
	exec nam out1.nam &
	exec awk -f throughput1.awk < outall1.tr > throughput1.tr
	#can add xgrpah as well
} 

# procedure to monitor current window
proc monitor-tcp-cwnd { src } {
	#the src here is the source's tcp object
	global cf ns
	set cwnd [ $src set cwnd_ ]
	set now [$ns now]
	#write to file x value , y value
	puts $cf "$now $cwnd"
	set time .1
	#set timer to recall this function
	$ns at [expr $now + $time] "monitor-tcp-cwnd $src"
}
#set n0
set tcp [new Agent/TCP]
$tcp set class_ 1
$ns attach-agent $n0 $tcp
$tcp set packet_size_ 500
set ftp [new Application/FTP]
$ftp attach-agent $tcp

#set sink
set sink [new Agent/TCPSink]
$ns attach-agent $n2 $sink

#connect src and sink
$ns connect $tcp $sink
#set src
set src $ftp

#set simulation
$ns at 1 "$src start"
$ns at 1 "monitor-tcp-cwnd $tcp"
$ns at 30 "$src stop"
$ns at 30 "finish"

#run simulation
$ns run


