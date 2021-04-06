set ns [new Simulator]
set nf [open out.nam w]
$ns namtrace-all $nf
set tf [open outall.tr w]
$ns trace-all $tf
set qs [open qsize.tr w]
set fp1 [open cwnd1.tr w]
set fp2 [open cwnd2.tr w]
$ns color 1 Blue
$ns color 2 Red

set n0 [$ns node]
set n1 [$ns node] 
set n2 [$ns node]
set n3 [$ns node] 

$ns duplex-link $n0 $n1 2Mb 10ms DropTail
$ns duplex-link $n1 $n2 1Mb 10ms DropTail
$ns duplex-link $n3 $n1 2Mb 10ms DropTail
$ns queue-limit $n1 $n2 20
set qmon1 [$ns monitor-queue $n1 $n2 [open qtrace.tr w] 0.03 ]
[$ns link $n1 $n2] queue-sample-timeout
$ns duplex-link-op $n1 $n2 queuePos 0.5

proc finish {} {
	global ns nf tf qs fp1 fp2
	$ns flush-trace
	close $nf
	close $tf
	close $qs
	close $fp1
	close $fp2
	exec nam out.nam &
	exec awk -f throughput.awk < outall.tr > throughput.tr &
	exit 0 
}

proc monitor-tcp-cwnd { src fp } {
	#the src here is the source's tcp object
	global ns
	set cwnd [ $src set cwnd_ ]
	set now [$ns now]
	#write to file x value , y value
	puts $fp "$now $cwnd"
	set time .05
	#set timer to recall this function
	$ns at [expr $now + $time] "monitor-tcp-cwnd $src $fp"
}

proc ftp-agent { node sink size class fp } {
	set ns [Simulator instance]
	set tcp [new Agent/TCP]
	$tcp set class_ $class
	$ns attach-agent $node $tcp
	$tcp set packet_size_ $size
	set ftp [new Application/FTP]
	$ftp attach-agent $tcp
	$ns connect $tcp $sink
	$ns at 1.0 "monitor-tcp-cwnd $tcp $fp"
	return $ftp
}

proc qsize { monitor } {
	global ns qs
	set size [$monitor set pkts_ ]
	set t [$ns now] 
	puts $qs "$t $size"
	set time .05
	$ns at [expr $t + $time ] "qsize $monitor"
}

set sink0 [new Agent/TCPSink]
set sink1 [new Agent/TCPSink]
$ns attach-agent $n2 $sink0
$ns attach-agent $n2 $sink1


set src0 [ftp-agent $n0 $sink0 500 1 $fp1 ]
set src1 [ftp-agent $n3 $sink1 500 2 $fp2 ]


$ns at 1.0 "$src0 start"
$ns at 1.0 "$src1 start"
$ns at 1.0 "qsize $qmon1"
$ns at 60.0 "$src0 stop"
$ns at 60.0 "$src1 stop" 
$ns at 60.0 "finish"


$ns run

