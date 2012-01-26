<?php

exec("ifconfig | grep Bcast | cut -d \":\" -f 3 | cut -d \" \" -f 1",$addr);
$addr=join($addr);
$msg='<bdp1 cmd="discover" application="iphone_remote" version="1" challenge="alohathere!" signature="51d3feb33925ce8aa108ec95d8a841ae"/>';
$port = 2562;

$sock = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
socket_bind($sock, "0.0.0.0");
socket_set_option($sock, SOL_SOCKET, SO_BROADCAST, 1);
socket_sendto($sock, $msg, strlen($msg), 0, $addr, $port);
echo("Waiting...");
$rec = "";
$l = socket_recv($sock, $rec, 2048, 0);
echo $l."\n";
echo $rec."\n";

socket_close($sock);


?>
