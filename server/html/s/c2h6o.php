<?php

$servername = "localhost";
$username = "bruconbadges_php";
$password = "xxxx";
$conn = mysqli_connect($servername, $username, $password,"bruconbadges");
if (!$conn) {
    die("Connection failed: " . mysqli_connect_error());
}
$mac = $_SERVER["SSL_CLIENT_S_DN_CN"];

if (array_key_exists("reading",$_GET))
    {
        $stmt = $conn->prepare("insert into alc_reading (mac,time_reading,reading) values (?,NOW(),?)");
        $stmt->bind_param("si", $mac,$_GET["reading"]);
        $stmt->execute();
        $stmt->close();
    }
else
    {
        echo "?";
    }

$conn->close();
?>