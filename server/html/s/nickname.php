<?php

$servername = "localhost";
$username = "bruconbadges_php";
$password = "xxx";
$conn = mysqli_connect($servername, $username, $password,"bruconbadges");
if (!$conn) {
    die("Connection failed: " . mysqli_connect_error());
}
$mac = $_SERVER["SSL_CLIENT_S_DN_CN"];

if (array_key_exists("nick",$_GET))
    {
        $nick = htmlspecialchars($_GET["nick"],ENT_QUOTES)            ;
        $stmt = $conn->prepare("update badges set username=? where mac=?");
        $stmt->bind_param(
            "ss",
            $nick,
            $mac
        );
        $stmt->execute();
#        echo mysqli_stmt_error ( $stmt );
        $stmt->close();
    }


$conn->close();
?>