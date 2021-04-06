set ns [new Simulator]
set nf [open out.nam w]
$ns namtrace-all $nf
set tf [open outall.tr w]
$ns trace-all $tf
$ns color 1 Blue
$ns color 2 Red

for {set i 0} {$i < 7} {incr i} {
	set n($i) [$ns node]
}
for {set i 0} {$i < 7} {incr i} {
	$ns duplex-link $n($i) $n([expr ($i+1)%7]) 1Mb 10ms DropTail
}


proc finish {} {
	global ns nf tf 
	$ns flush-trace
	close $nf
	close $tf
	exec nam out.nam &
	exit 0 
}
proc cbr-agent { node sink size interval } {
	set ns [Simulator instance]
	set udp [new Agent/UDP]
	$ns attach-agent $node $udp
	$udp set class_ 1 
	set traffic [new Application/Traffic/CBR]
	$traffic set packetSize_ $size
	$traffic set interval_ $interval
	$traffic attach-agent $udp
	$ns connect $udp $sink
	return $traffic
}

set sink0 [new Agent/LossMonitor]
$ns attach-agent $n(3) $sink0
set src0 [cbr-agent $n(0) $sink0 500 0.05 ]

$ns rtmodel-at 1.0 down $n(1) $n(2)
$ns rtmodel-at 2.0 up $n(1) $n(2)
$ns at 0.1 "$src0 start"
$ns at 5.0 "$src0 stop"
$ns at 5.0 "finish"
$ns run


