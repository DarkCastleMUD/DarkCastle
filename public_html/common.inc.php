<?php
$host = "localhost";
$uri = "/";
$port = 8888;

require('utils.php');

session_start();

function rpc_call($method, $args) {
  global $host, $uri, $port;

  if (empty($_SESSION['login']) || empty($_SESSION['password'])) {
    return 0;
  }

  $result = xu_rpc_http_concise(array('method' => $method,
				      'args' => array($_SESSION['login'], $_SESSION['password'], $args),
				      'host'  => $host,
				      'uri'  => $uri,
				      'port'  => $port));

  if (empty($result))
    $result = 'connection failed';

  return $result;
}

function authenticated()
{
  $result = rpc_call("login", null);
  if (is_string($result))
    $_SESSION['status'] = $result;

  if ($result === 'authorized') {
    return 1;
  } else {
    return 0;
  }
}

function get_editor_type()
{
  $type = rpc_call("get_editor_type", null);

  return $type;
}

function editor($arg1)
{
  return rpc_call("editor", $arg1);
}
?>