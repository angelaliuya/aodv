#============================================================================
#
#=============================================================================
set opt(namfile)          reactive.nam
set opt(tracefile)        reactive.tr
set opt(traffic)         "traffic/cbr-15-1-5-5-e"
set opt(x)               800
set opt(y)               500
set opt(wirelessNodes)   15
set opt(gatewayNodes)    2
set opt(wiredNodes)      4
set opt(mobility)        "mobility/15-5-10-900-800-500-a"
set opt(stop)            50
set opt(gw_discovery)    reactive



# ==========================================================================
# Movement trace
# ==========================================================================
proc log-mn-movement { } {
    global logtimer ns
    
    source  /home/liuya/ns/ns-2.33/tcl/mobility/timer.tcl

    Class LogTimer -superclass Timer
    LogTimer instproc timeout {} {
 	global mobile6 mobile7 mobile8 mobile9 mobile10 mobile11 mobile12 mobile13 mobile14 mobile15 mobile16 mobile17 mobile18 mobile19 mobile20 
	$mobile6 log-movement
	$mobile7 log-movement
	$mobile8 log-movement
	$mobile9 log-movement
	$mobile10 log-movement
	$mobile11 log-movement
	$mobile12 log-movement
	$mobile13 log-movement
	$mobile15 log-movement
	$mobile16 log-movement
	$mobile17 log-movement
	$mobile18 log-movement
	$mobile19 log-movement
	$mobile20 log-movement
	
	$self sched 1 
    }
    set logtimer [new LogTimer]
    $logtimer sched 1  
}
# ==========================================================================
# Main Program
# ==========================================================================
proc create-topo {} {
    global ns opt topo mobile6 mobile7 mobile8 mobile9 mobile10 mobile11 mobile12 mobile13 mobile14 mobile15 mobile16 mobile17 mobile18 mobile19 mobile20 gw1 gw2 router1 router2 host1 host2
    
    #create a topology object and define topology
    set topo [new Topography]
    $topo load_flatgrid $opt(x) $opt(y)
    set god [create-god [expr $opt(wirelessNodes)+$opt(gatewayNodes)]]

    #---------Addressing---------#
    #For mixed simulations we need to use hierarchical routing in order to 
    #route packets. Must be put before addressing starts.
    $ns node-config -addressType hierarchical
    
    #Number of domains
    #4 domains
    AddrParams set domain_num_ 4
    
    #Number of clusters in each domain
    #4 clusters 
    #1 cluster in domain 0 
    #1 cluster in domain 1 
    #1 cluster in domain 2 
    #1 cluster in domain 3 
    AddrParams set cluster_num_ {1 1 1 1}

    #Number of nodes in each cluster
    #21 nodes
    #2 nodes in domain 0 in cluster 0: router1 0.0.0  host1 0.0.1
    #2 nodes in domain 1 in cluster 0: router2 1.0.0  host2 1.0.1
    #9 nodes in domain 2 in cluster 0: gw1 2.0.0  mobile6-mobile13 2.0.1-2.0.8
    #8 nodes in domain 3 in cluster 0: gw2 3.0.0  mobile14-mobile20 3.0.1-3.0.7
    AddrParams set nodes_num_ {2 2 9 8}
    
    #Chooses method for gateway discovery
    #NOTE! This assumes that AODV is used and that GATEWAY_DISCOVERY is defined
    #in aodv.h!!!
    if {$opt(gw_discovery) == "proactive"} {
	Agent/AODV set gw_discovery 0
    }
    if {$opt(gw_discovery) == "hybrid"} {
	Agent/AODV set gw_discovery 1
    } 
    if {$opt(gw_discovery) == "reactive"} {
	Agent/AODV set gw_discovery 2
    }

    
    #--------------------------
    # Create Nodes
    #--------------------------
    ### Create wired nodes
    set router1 [$ns node 0.0.0]
    set host1 [$ns node 0.0.1]
    set router2 [$ns node 1.0.0]
    set host2 [$ns node 1.0.1]
    
    # Configure for mobile nodes and gateways
    $ns node-config -adhocRouting AODV
    $ns node-config -llType LL
    $ns node-config -macType Mac/802_11
    $ns node-config -ifqType Queue/DropTail/PriQueue
    $ns node-config -ifqLen  50
    $ns node-config -antType Antenna/OmniAntenna
    $ns node-config -propType Propagation/TwoRayGround
    $ns node-config -phyType  Phy/WirelessPhy
    $ns node-config -topoInstance $topo
    $ns node-config -channel [new Channel/WirelessChannel]
    $ns node-config -agentTrace ON
    $ns node-config -routerTrace OFF
    $ns node-config -macTrace OFF
    $ns node-config -movementTrace OFF
    
    # Configure for gateways
    $ns node-config -wiredRouting ON
    
    ### Create gateways
    set gw1 [$ns node 2.0.0]
    set gw2 [$ns node 3.0.0]
    
    $gw1 set X_ 100.0
    $gw1 set Y_ 250.0
    $gw1 set Z_ 0.0
    $gw2 set X_ 700.0
    $gw2 set Y_ 250.0
    $gw2 set Z_ 0.0
    
    $ns at 0.00 "$gw1 setdest 100 250 0"
    $ns at 0.00 "$gw2 setdest 700 250 0"
#ly修改************************************************************
  #获得路由层协议
  #set rt1 [$gw1 agent 255]
  #set rt2 [$gw2 agent 255]
  #/初始化mac对象
  #$rt1 set-mac [$gw1 set mac_(0)]
  #$rt2 set-mac [$gw2 set mac_(0)]
#*****************************************************************

#set rt($i) [$node_($i) agent 255] #获得路由层协议
#$rt($i) set-mac [$node_($i) set mac_(0)]  #初始化mac对象

 
    # Configure for mobile nodes
    $ns node-config -wiredRouting OFF
    
    ### Create mobile nodes
    set temp {2.0.1 2.0.2 2.0.3 2.0.4 2.0.5 2.0.6 2.0.7 2.0.8 \
	3.0.1 3.0.2 3.0.3 3.0.4 3.0.5 3.0.6 3.0.7}   
    for {set i 6} {$i < $opt(wirelessNodes)+6 } {incr i} {
	set mobile$i [$ns node [lindex $temp [expr $i-6]]]
    }
    
    $mobile6 base-station [AddrParams addr2id [$gw1 node-addr]]
    $mobile7 base-station [AddrParams addr2id [$gw1 node-addr]]
    $mobile8 base-station [AddrParams addr2id [$gw1 node-addr]]
    $mobile9 base-station [AddrParams addr2id [$gw1 node-addr]]
    $mobile10 base-station [AddrParams addr2id [$gw1 node-addr]]
    $mobile11 base-station [AddrParams addr2id [$gw1 node-addr]]
    $mobile12 base-station [AddrParams addr2id [$gw1 node-addr]]
    $mobile13 base-station [AddrParams addr2id [$gw1 node-addr]]
    
    $mobile14 base-station [AddrParams addr2id [$gw2 node-addr]]
    $mobile15 base-station [AddrParams addr2id [$gw2 node-addr]]
    $mobile16 base-station [AddrParams addr2id [$gw2 node-addr]]
    $mobile17 base-station [AddrParams addr2id [$gw2 node-addr]]
    $mobile18 base-station [AddrParams addr2id [$gw2 node-addr]]
    $mobile19 base-station [AddrParams addr2id [$gw2 node-addr]]
    $mobile20 base-station [AddrParams addr2id [$gw2 node-addr]]
    
    # Mobility model generated by setdest
    source $opt(mobility)
    
    $ns at 0.0 "$router1 label \"ROUTER\""
    $ns at 0.0 "$router2 label \"ROUTER\""
    $ns at 0.0 "$host1 label \"HOST\""
    $ns at 0.0 "$host2 label \"HOST\""
    $ns at 0.0 "$gw1 label \"GATEWAY\""
    $ns at 0.0 "$gw2 label \"GATEWAY\""
    $ns at 0.0 "$mobile6 label \"MN 6\""
    $ns at 0.0 "$mobile7 label \"MN 7\""
    $ns at 0.0 "$mobile8 label \"MN 8\""    
    $ns at 0.0 "$mobile9 label \"MN 9\""
    $ns at 0.0 "$mobile10 label \"MN 10\""
    $ns at 0.0 "$mobile11 label \"MN 11\""    
    $ns at 0.0 "$mobile12 label \"MN 12\""
    $ns at 0.0 "$mobile13 label \"MN 13\""
    $ns at 0.0 "$mobile14 label \"MN 14\""    
    $ns at 0.0 "$mobile15 label \"MN 15\""
    $ns at 0.0 "$mobile16 label \"MN 16\""
    $ns at 0.0 "$mobile17 label \"MN 17\""
    $ns at 0.0 "$mobile18 label \"MN 18\""
    $ns at 0.0 "$mobile19 label \"MN 19\""
    $ns at 0.0 "$mobile20 label \"MN 20\"" 

    $ns initial_node_pos $mobile7 20
    $ns initial_node_pos $mobile8 20
    $ns initial_node_pos $mobile9 20
    $ns initial_node_pos $mobile10 20
    $ns initial_node_pos $mobile11 20
    $ns initial_node_pos $mobile12 20
    $ns initial_node_pos $mobile13 20
    $ns initial_node_pos $mobile14 20
    $ns initial_node_pos $mobile15 20
    $ns initial_node_pos $mobile16 20
    $ns initial_node_pos $mobile17 20
    $ns initial_node_pos $mobile18 20
    $ns initial_node_pos $mobile19 20
    $ns initial_node_pos $mobile20 20
    
    $router1 color blue
    $router2 color blue
    $host1 color blue
    $host2 color blue
    $gw1 color red
    $gw2 color red
    $gw1 shape hexagon
    $gw2 shape hexagon
    $router1 shape square
    $router2 shape square
    $host1 shape box
    $host2 shape box
    
    $ns duplex-link $router1 $router2 100Mb 1.80ms DropTail
    $ns duplex-link-op $router1 $router2 orient right
    $ns duplex-link $router1 $host1 100Mb 1.80ms DropTail
    $ns duplex-link-op $router1 $host1 orient down
    $ns duplex-link $router2 $host2 100Mb 1.80ms DropTail
    $ns duplex-link-op $router2 $host2 orient down
    $ns duplex-link-op $router1 $router2 queuePos 0.5

    $ns duplex-link $router1 $gw1 100Mb 2ms DropTail
    $ns duplex-link-op $router1 $gw1 orient up
    $ns duplex-link $router2 $gw2 100Mb 2ms DropTail
    $ns duplex-link-op $router2 $gw2 orient up
   
}

################################################################
# End of Simulation
################################################################
proc finish { } {
    global tracef ns namf opt mobile6 mobile7 mobile8 mobile9 mobile10 mobile11 mobile12 mobile13 mobile14 mobile15 mobile16 mobile17 mobile18 mobile19 mobile20 gw1 gw2 router1 router2 host1 host2
    
    $ns at $opt(stop).01 "$router1 reset";
    $ns at $opt(stop).01 "$host1 reset";
    $ns at $opt(stop).01 "$router2 reset";
    $ns at $opt(stop).01 "$host2 reset";
    
    $ns at $opt(stop).01 "$gw1 reset";
    $ns at $opt(stop).01 "$gw2 reset";
    
    $ns at $opt(stop).01 "$mobile6 reset";
    $ns at $opt(stop).01 "$mobile7 reset";
    $ns at $opt(stop).01 "$mobile8 reset";
    $ns at $opt(stop).01 "$mobile9 reset";
    $ns at $opt(stop).01 "$mobile10 reset";
    $ns at $opt(stop).01 "$mobile11 reset";
    $ns at $opt(stop).01 "$mobile12 reset";
    $ns at $opt(stop).01 "$mobile13 reset";
    $ns at $opt(stop).01 "$mobile14 reset";
    $ns at $opt(stop).01 "$mobile15 reset";
    $ns at $opt(stop).01 "$mobile16 reset";
    $ns at $opt(stop).01 "$mobile17 reset";
    $ns at $opt(stop).01 "$mobile18 reset";
    $ns at $opt(stop).01 "$mobile19 reset";
    $ns at $opt(stop).01 "$mobile20 reset";
    
    $ns at $opt(stop).01 "$ns nam-end-wireless $opt(stop)"
    $ns at $opt(stop).0002 "$ns halt"
    
    $ns flush-trace
    flush $tracef
    close $tracef
    close $namf
    puts "Running nam with $opt(namfile) ... "
    exec nam $opt(namfile) &
    exit 0
}

################################################################
# Main 
################################################################
proc main { } {
    global opt ns namf tracef
    
    set ns [new Simulator]
    $ns color 0 Brown
    $ns color 1 Blue
    $ns color 2 Yellow
    $ns color 3 Green
    $ns color 4 Purple
    $ns color 5 Red
    
   #set tracef [open $opt(tracefile) w]
   #$ns trace-all $tracef
   #set namf [open $opt(namfile) w]
   #$ns namtrace-all-wireless $namf $opt(x) $opt(y)



    #$ns use-newtrace
    exec rm -f $opt(tracefile)
    
    set tracef [open $opt(tracefile) w]
    $ns trace-all $tracef
    set namf [open $opt(namfile) w]
    $ns namtrace-all-wireless $namf $opt(x) $opt(y)
    
   create-topo
    log-mn-movement
    set-traffic
    
    #>----------------------- Run Simulation -------------------------<
    $ns at $opt(stop).0001 "finish"
    $ns at 0.0 "$ns set-animation-rate 15ms"
    # For tracegraph
    puts $tracef "mixed12 ip hex"

    puts "Starting simulation..."
    puts "Please wait..."
    $ns run
    
}

set opt(start-src) 1
set opt(stop-src) [expr $opt(stop)-1]
proc set-traffic { } {
    global ns opt mobile6 mobile7 mobile8 mobile9 mobile10 mobile11 mobile12 mobile13 mobile14  mobile15 mobile16 mobile17 mobile18 mobile19 mobile20 router1 router2 host1 host2
     
    
    set opt(agent) cbr-gen
    
    if {$opt(agent) == "cbr-gen"} {
   	source $opt(traffic)
    }
    
    if {$opt(agent) == "cbr-gen-mobile"} {
   	source ./traffic/more_traffic/cbr-15-1-5-5-mobile
    }
    
    if {$opt(agent) == "cbr"} {
	set src [new Agent/UDP]
	set dst [new Agent/Null]
	$ns attach-agent $mobile7 $src 
	$ns attach-agent $host1 $dst
	$ns connect $src $dst       
	
	set cbr [new Application/Traffic/CBR]
        $cbr attach-agent $src
	$cbr set packetSize_ 512
	$cbr set interval_ 0.1
	#$cbr set packetSize_ 1000
	#$cbr set interval_ 0.0178571429
	$ns at $opt(start-src) "$cbr start"
	$ns at $opt(stop-src) "$cbr stop"
    }
    
    if {$opt(agent) == "tcp"} {
	set src [new Agent/TCP]
	set dst [new Agent/TCPSink]
	$ns attach-agent $mobile6 $src
	$ns attach-agent $mobile10 $dst
	$ns connect $src $dst
	
	$src set class_ 2
	set ftp [new Application/FTP]
	$ftp attach-agent $src
	$ns at $opt(start-src) "$ftp start"
	$ns at $opt(stop-src) "$ftp stop"
    }
 
    if {$opt(agent) == "exp" || $opt(agent) == "poi" } {
	set src [new Agent/UDP]
        set dst [new Agent/UDP]
        $ns attach-agent $mobile6 $src
        $ns attach-agent $mobile8 $dst
        $ns connect $src $dst
	
        set exp [new Application/Traffic/Exponential]
        $exp attach-agent $src
        $exp set packetSize_ 210
	$exp set idle_time_ 500ms
	
	if {$opt(agent) == "exp"} {
	    $exp set burst_time_ 0
	    $exp set rate_ 10000k 
	} else {
	    $exp set burst_time_ 500ms
	    $exp set rate_ 100k
	}
	
	$ns at $opt(start-src) "$exp start"
	#$ns at $opt(stop-src) "$exp stop"
    }
} 

main
