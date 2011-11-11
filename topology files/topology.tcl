set val(chan)       Channel/WirelessChannel
set val(prop)       Propagation/TwoRayGround
set val(netif)      Phy/WirelessPhy
set val(mac)        Mac/802_11
set val(ifq)        Queue/DropTail/PriQueue
set val(ll)         LL
set val(ant)        Antenna/OmniAntenna
set val(x)              560  ;# X dimension of the topography
set val(y)              280   ;# Y dimension of the topography
set val(ifqlen)         100            ;# max packet in ifq
set val(seed)           0.0
set val(rp)   AODV
set val(nn)             120            ;# how many nodes are simulated
#set val(cp)             "../mobility/scene/cbr-3-test" 
#set val(sc)             "../mobility/scene/scen-3-test" 
set val(stop)           21.0           ;# simulation time
set val(stop)          21.0           ;# simulation time

set val(row)		8
set val(col)		15
#set val(cp)             "/home/prateek/Desktop/ns_project/scene"
set val(cp)             "/home/prateek/Desktop/ns_project/scene1"
#-------Event scheduler object creation--------#

set ns [new Simulator]

# Creating trace file and nam file

set tracefd [open wireless1.tr w]
set namtrace [open wireless1.nam w]   

$ns trace-all $tracefd
$ns namtrace-all-wireless $namtrace $val(x) $val(y)


# define different colors for nam data flows
#$ns color 0 Green




# set up topography object
set topo [new Topography]
$topo load_flatgrid $val(x) $val(y)

set god_ [create-god $val(nn)]

# configure the nodes
        $ns node-config -adhocRouting $val(rp) \
                   -llType $val(ll) \
                   -macType $val(mac) \
                   -ifqType $val(ifq) \
                   -ifqLen $val(ifqlen) \
                   -antType $val(ant) \
                   -propType $val(prop) \
                   -phyType $val(netif) \
                   -channelType $val(chan) \
                   -topoInstance $topo \
                   -agentTrace ON \
                   -routerTrace ON \
                   -macTrace OFF \
                   -movementTrace ON

#Phy/WirelessPhy set Pt_ 8.5872e-4; # For 40m transmission range.   

#Phy/WirelessPhy set Pt_ 7.214e-3;  # For 100m transmission range.   
#Phy/WirelessPhy set Pt_ 7.214e-1;  # my own

Phy/WirelessPhy set Pt_  0.2818; # For 250m transmission range.
## Creating node objects...        
for {set i 0} {$i < $val(nn) } { incr i } {
            set node_($i) [$ns node]
     
      }
     # for {set i 0} {$i < $val(nn)  } {incr i } {
      #      $node_($i) color black
       #     $ns at 0.0 "$node_($i) color black"
      #}

# Provide initial location of mobile nodes
set k 0
for {set i 0} {$i < $val(row) } { incr i } {
	for {set j 0} {$j < $val(col) } { incr j } {
	set x [expr {$j*70.0}]
	set y [expr {$i*70.0}]
	
	$node_($k) set X_ $x
	$node_($k) set Y_ $y
	$node_($k) set Z_ 0.0
	incr k
}
	
}


# Define node initial position in nam
for {set i 0} {$i < $val(nn)} { incr i } {
$ns initial_node_pos $node_($i) 20
}

$ns at 5.0 "[$node_(62) agent 255] start_mesh";
$ns at 5.9 "[$node_(89) agent 255] high_priority";
$ns at 5.9 "[$node_(74) agent 255] high_priority";
$ns at 5.9 "[$node_(59) agent 255] high_priority";

#$node_(59) set priority 1

puts "Loading connection pattern..."
source $val(cp)

for {set i 0} {$i < $val(nn) } { incr i } {
            $ns at 20.0 "[$node_($i) agent 255] print_mesh"
}	

#$node_(45) set priority 1
#$node_(48) set priority 1
#$node_(15) set priority 1
#$node_(51) set priority 1
#$node_(69) set mesh 1


# Telling nodes when the simulation ends
for {set i 0} {$i < $val(nn) } { incr i } {
    $ns at $val(stop) "$node_($i) reset";
}

# Ending nam and the simulation
$ns at $val(stop) "$ns nam-end-wireless $val(stop)"
$ns at $val(stop) "stop"
$ns at 20.01 "puts \"end simulation\"; $ns halt"
#stop procedure:
proc stop {} {
    global ns tracefd namtrace
    $ns flush-trace
    close $tracefd
    close $namtrace
exec nam wireless1.nam &
}

$ns run
# How to run the program


# snapshot of the program output

