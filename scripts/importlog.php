#!/usr/bin/php
<?php
class socket_log {
  const CONNECT_NORMAL=1;
  const DISCONNECT_NORMAL=2;
  const CONNECT_FAIL_PASSWORD=3;
  const DISCONNECT_LOSING_PLAYER=4;
  const DISCONNECT_PEER_EOF=5;
  const DISCONNECT_PEER_BAILED=6;
  const CONNECT_RECONNECT=7;
  const CONNECT_NEW_PLAYER=8;
  const OTHER_REBOOT=9;
  const OTHER_PASSWORD_CHANGED=10;
  const DISCONNECT_INPUT_OVERFLOW=11;
  const CONNECT_PASSWORD_EXCESSIVE=12;
  const CONNECT_DENIED=13;
  const DISCONNECT_NORMAL_CQ=14;
  public $db;
  public $sth;
  public $verbose;

  public function add_entry($date, $data, $reason) {
    switch($reason) {
    case self::CONNECT_NORMAL:
    case self::CONNECT_FAIL_PASSWORD:
    case self::CONNECT_RECONNECT:
    case self::CONNECT_NEW_PLAYER:
      $type = 1;
      $name = $data[1];
      $ip = $data[2];
      $room = 0;
      break;
    case self::CONNECT_PASSWORD_EXCESSIVE:
      $type = 1;
      $name = $data[1];
      $ip = '0.0.0.0';
      $room = 0;
      break;     
    case self::DISCONNECT_NORMAL:
      $type = 2;
      $name = $data[1];
      $ip = '0.0.0.0';
      $room = $data[2];
      break;
    case self::DISCONNECT_NORMAL_CQ:
      $type = 2;
      $name = $data[1];
      $ip = '0.0.0.0';
      $room = $data[2];
      break;
    case self::DISCONNECT_LOSING_PLAYER:
      $type = 2;
      $name = $data[1];
      $ip = '0.0.0.0';
      $room = 0;
      break;
    case self::DISCONNECT_PEER_EOF:
    case self::DISCONNECT_INPUT_OVERFLOW:
      $type = 2;
      $name = '-';
      $ip = '0.0.0.0';
      $room = 0;
      break;

    case self::OTHER_REBOOT:
      $type = 3;
      $name = '-';
      $ip = '0.0.0.0';
      $room = 0;
      break;
    case self::OTHER_PASSWORD_CHANGED:
      $type = 3;
      $name = $data[1];
      $ip = '0.0.0.0';
      $room = 0;
      break;

    case self::DISCONNECT_PEER_BAILED:
      $type = 2;
      $name = '-';
      $ip = $data[1];
      $room = 0;
      break;

    case self::CONNECT_DENIED:
      $type = 1;
      $name = '-';
      $ip = $data[1];
      $room = 0;
      break;
    default:

      break;
    }
    
    if (! isset($this->sth)) {
      $this->sth = pg_prepare($this->db, 'my_query', 'INSERT INTO socket_log VALUES ($1, $2, $3, $4, $5, $6)');
      if ($this->sth === FALSE) {
	echo 'error on prepare';
	return;
      }
    }
    
    $sql_array = array($date, $name, $ip, $type, $reason, $room);

    $res = @pg_execute($this->db, 'my_query', $sql_array);
    if ($this->verbose == TRUE &&
	($res == FALSE || pg_result_status($res) != PGSQL_COMMAND_OK)) {
      echo "\n" . pg_last_error($this->db) . "\n";
      echo $date . " " . $data[0] . "\n";
    }
  }

  public function main(){

    $argc = $_SERVER['argc'];
    $argv = $_SERVER['argv'];
    
    if ($argc < 2) {
      $this->usage();
      die(1);
    }
    
    if ($argc > 2 && $argv[1] == '-v') {
      $filename = $argv[2];
      $this->verbose = TRUE;
    } else {
      $filename = $argv[1];
      $this->verbose = FALSE;   
    }

    $this->db = pg_connect('dbname=dcastle user=dcastle password=wm42LyP1 host=localhost');
    if ($this->db === FALSE) {
      die('Could not connect: ' . pg_last_error());
    }

    $fd = @fopen($filename, 'r');
    if ($fd == null) {
      die("Error opening: ".$argv[1]."\n");
    }

    $total_lines=0;
    while(!feof($fd)) {
      $total_lines++;
      fgets($fd);
    }

    rewind($fd);

    $current_line_num=0;
    while (!feof($fd)) {
      $current_line_num++;

      //      if ($current_line_num % $total_lines == 0) {
      printf("%.2f%% of file read...\r",($current_line_num/$total_lines)*100.00);
	//      }

      $line = fgets($fd);
      if ($line == FALSE) {
	echo "\n$current_line_num of $total_lines parsed.\n";
	die("Could not read from file");
      } else {
	list($date, $entry) = explode(" :: ", $line);
	
	$a = memory_get_usage(TRUE);
	if (preg_match('/^(.+)@(.+) has connected.$/', $entry, $match)) {
	  $this->add_entry($date, $match, self::CONNECT_NORMAL);
	} elseif (preg_match('/^Closing link to: (.+) at (.+). with CQ.$/', $entry, $match)) {
	  $this->add_entry($date, $match,  self::DISCONNECT_NORMAL_CQ);
	} elseif (preg_match('/^Closing link to: (.+) at (.+).$/', $entry, $match)) {
	  $this->add_entry($date, $match,  self::DISCONNECT_NORMAL);
	} elseif (preg_match('/^(.+) wrong password: (.+)$/', $entry, $match)) {
	  $this->add_entry($date, $match, self::CONNECT_FAIL_PASSWORD);
	} elseif (preg_match('/^Losing player: (.+).$/', $entry, $match)) {
	  $this->add_entry($date, $match,  self::DISCONNECT_LOSING_PLAYER);
	} elseif (preg_match('/^EOF on socket read \(connection broken by peer\)$/', $entry, $match)) {
	  $this->add_entry($date, $match, self::DISCONNECT_PEER_EOF);
	} elseif (preg_match('/^Connection attempt bailed from (.+)$/', $entry, $match)) {
	  $this->add_entry($date, $match, self::DISCONNECT_PEER_BAILED);
	} elseif (preg_match('/^(.+)@(.+) has reconnected.$/', $entry, $match)) {
	  $this->add_entry($date, $match, self::CONNECT_RECONNECT);
	} elseif (preg_match('/^(.+)@(.+) new player.$/', $entry, $match)) {
	  $this->add_entry($date, $match, self::CONNECT_NEW_PLAYER);
	} elseif (preg_match('/^\*\*\*\*\*\*\*\*\*\*\*\*\*\* REBOOTING THE MUD \*\*\*\*\*\*\*\*\*\*\*$/', $entry, $match)) {
	  $this->add_entry($date, $match, self::OTHER_REBOOT);
	} elseif (preg_match('/^(.+) password changed$/', $entry, $match)) {
	  $this->add_entry($date, $match, self::OTHER_PASSWORD_CHANGED);
	} elseif (preg_match('/^process_input: about to close connection: input overflow$/', $entry, $match)) {
	  $this->add_entry($date, $match, self::DISCONNECT_INPUT_OVERFLOW);
	} elseif (preg_match('/^(.+) has 100\+ bad pw tries...$/', $entry, $match)) {
	  $this->add_entry($date, $match, self::CONNECT_PASSWORD_EXCESSIVE);
	} elseif (preg_match('/^Connection attempt denied from \[(.+)\]$/', $entry, $match)) {
	  $this->add_entry($date, $match, self::CONNECT_DENIED);
	} else {
	  die('Unable to parse line: ' . $entry . "\n");
	}
      }
    }
  }


  public function usage() {
    global $argv, $argc;
    echo 'Usage: ' . basename($argv[0]) . " [-v] <filename>\n\n";
  }

}

$sl = new socket_log();
$sl->main();

?>
