set ns [new Simulator]
set nf [open out.nam w]
$ns namtrace-all $nf
set tf [open outall.tr w]
$ns trace-all $tf
set pkloss [open pkloss.tr w]

$ns color 1 Blue
$ns color 2 Red

set n0 [$ns node]
set n1 [$ns node] 
set n2 [$ns node]
set n3 [$ns node]2Mb 100ms

$ns duplex-link $n0 $n2 2Mb 10ms DropTail
$ns duplex-link $n1 $n2 2Mb 10ms DropTail
$ns duplex-link $n2 $n3 2Mb 10ms DropTail

proc finish {} {
	global ns nf pkloss latency tf
	$ns flush-trace
	close $nf
	close $pkloss
	close $tf
	exec nam out.nam &
	exec awk -f latency.awk < outall.tr > latency.tr &
	exec awk -f throughput.awk < outall.tr > throughput.tr &
	exit 0 
}

proc monitor-udp-loss { sink } {
	#need Loss monitor sink
	global ns pkloss
	set n [$sink set  nlost_ ]
	set now [$ns now]
	puts $pkloss "$now $n"
	set time .1
	$ns at [expr $now + $time ] "monitor-udp-loss $sink"
}

proc ftp-agent { node sink size } {
	set ns [Simulator instance]
	set tcp [new Agent/TCP]
	$tcp set class_ 1 
	$ns attach-agent $node $tcp
	$tcp set packet_size_ $size
	set ftp [new Application/FTP]
	$ftp attach-agent $tcp
	$ns connect $tcp $sink
	return $ftp
}

proc cbr-agent { node sink size interval } {
	set ns [Simulator instance]
	set udp [new Agent/UDP]
	$ns attach-agent $node $udp
	$udp set class_ 2 
	set traffic [new Application/Traffic/CBR]
	$traffic set packetSize_ $size
	$traffic set interval_ $interval
	$traffic attach-agent $udp
	$ns connect $udp $sink
	return $traffic
}

set sink0 [new Agent/TCPSink]
set sink1 [new Agent/LossMonitor]
$ns attach-agent $n3 $sink0
$ns attach-agent $n3 $sink1


set src0 [ftp-agent $n1 $sink0 500 ]
set src1 [cbr-agent $n0 $sink1 500 0.0025 ]


$ns at 0.1 "$src0 start"
$ns at 5.0 "monitor-udp-loss $sink1"
$ns at 5.0 "$src1 start"

$ns at 10.0 "$src0 stop"
$ns at 10.0 "$src1 stop" 
$ns at 10.0 "finish"


$ns run

