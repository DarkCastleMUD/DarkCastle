#!/usr/bin/php
<?php

function player_connected($player) {
  global $connected, $connect_time, $state;

  $connected[$player]++;
  $state[$player] = 'connected';
  $connect_time[$player] = $timestamp;
}

function player_disconnected($player) {
  global $disconnected, $disconnect_time, $connect_time, $total_time;

  $disconnected[$player]++;
  $disconnect_time[$player] = $timestamp;
  
  if (isset($connect_time[$player]) && $disconnect_time[$player] > $connect_time[$player]) {
    $total_time[$player] += ($disconnect_time[$player] - $connect_time[$player]);
  }
}

function parse_log($line) {
  global $connected, $disconnected, $total_time, $connect_time, $disconnect_time, $first_timestamp, $last_timestamp;

  if ($line == null)
    return;

  $array = explode(' :: ', $line);
  $date = $array[0];
  $entry = $array[1];

  if (($timestamp = strtotime($date)) === false) {
    die('Error parsing date: ' . $date . "\n");
  }

  if (!isset($first_timestamp))
    $first_timestamp = $timestamp;
  else
    $last_timestamp = $timestamp;

  if (preg_match('/^(.+)@(.+) has connected.$/', $entry, $match)) {
    player_connected($match[1]);
  } elseif (preg_match('/^Closing link to: (.+) at (.+).$/', $entry, $match)) {
    player_disconnected($match[1]);
  } elseif (preg_match('/^(.+) wrong password: (.+)$/', $entry, $match)) {
  } elseif (preg_match('/^Losing player: (.+)$/', $entry, $match)) {
  } elseif (preg_match('/^EOF on socket read \(connection broken by peer\)$/', $entry, $match)) {
    // Set state only
  } elseif (preg_match('/^Connection attempt bailed from (.+)$/', $entry, $match)) {
  } elseif (preg_match('/^(.+)@(.+) has reconnected.$/', $entry, $match)) {

    player_connected($match[1]);
  } elseif (preg_match('/^(.+)@(.+) new player.$/', $entry, $match)) {
    player_connected($match[1]);
  } elseif (preg_match('/^\*\*\*\*\*\*\*\*\*\*\*\*\*\* REBOOTING THE MUD \*\*\*\*\*\*\*\*\*\*\*$/', $entry, $match)) {
  } elseif (preg_match('/^(.+) password changed$/', $entry, $match)) {
  } elseif (preg_match('/^process_input: about to close connection: input overflow$/', $entry, $match)) {
  } elseif (preg_match('/^(.+) has 100\+ bad pw tries...$/', $entry, $match)) {
  } elseif (preg_match('/^Connection attempt denied from \[(.+)\]$/', $entry, $match)) {
  } else {
    die('Unable to parse line: ' . $entry . "\n");
  }

}

{
  require_once('DB.php');

  $db =& DB::connect('pgsql://dcastle:xyz@localhost/dcastle', null);
  if (PEAR::isError($db)) {
    die($db->getMessage());
  }
  
  if ($_SERVER['argc'] < 3) {
    echo 'Usage: ', $_SERVER['argv'][0], ' [log filename] [mode]', "\n";
    echo 'Possible modes: ', "\n";
    echo ' * connected - Shows how many times each user was connected.', "\n";
    echo ' * length    - Shows how long each user stayed connected.', "\n";
    echo ' * player    - Shows info on a specific player', "\n";
    exit;
  } else {
    $cmd = $_SERVER['argv'][0];
    $logfile = $_SERVER['argv'][1];
    $mode = $_SERVER['argv'][2];
  }

  $fd = @fopen($logfile, 'r');
  if ($fd == null) {
    die("Error opening: $logfile\n");
  }

  while (!feof($fd)) {
    $line = fgets($fd);
    parse_log($line);

    //    if ($n++ > 1000)
    //      break;
  }

  echo "Between ", date("r", $first_timestamp), " and ", date("r", $last_timestamp), "...\n";

  if ($mode == 'connected') {
    echo "Top 10 list of number of times player was connected.\n";
    echo "----------------------------------------------------\n";
    arsort($connected);

    // Calculate largest length of player names in top 10
    foreach($connected as $player => $n) {
      if (++$i > 10)
	break;
      if (strlen($player) > $len)
	$len = strlen($player);
    }

    $i=0;
    foreach($connected as $player => $n) {
      if (++$i > 10)
	break;

      $format = "%2d. %${len}s connected %3d times.\n";
      printf($format, $i, $player, $n);
    }
  } elseif ($mode == 'length') {
    echo "Top 10 list of length of time player was connected\n";
    echo "--------------------------------------------------\n";
    arsort($total_time);

    // Calculate largest length of player names in top 10
    foreach($total_time as $player => $n) {
      if (++$i > 10)
	break;
      if (strlen($player) > $len)
	$len = strlen($player);
    }

    $i=0;
    foreach($total_time as $player => $n) {
      if (++$i > 10)
	break;

      // Calculate days, remaining hours, remaining minutes...
      $days = $n/60/60/24;
      $hours = $n/60/60%24;
      $mins = $n/60%60;
      $secs = $n%60;

      // Create specialized format string
      $format = "%2d. %${len}s has been connected: ";
      printf($format, $i, $player);
      if ($days >= 1)
	printf("%2d days ", $days);
      if ($hours)
	printf("%2d hours ", $hours);
      if ($mins)
	printf("%2d minutes ", $mins);
      if ($secs)
        printf("%2d seconds ", $secs);

      echo "(${connected[$player]} vs ${disconnected[$player]})\n";
    }
  }

  fclose($fd);
  $db->disconnect();
}
?>
