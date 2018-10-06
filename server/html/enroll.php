<?php

$servername = "localhost";
$username = "bruconbadges_php";
$password = "xxxx";

$conn = mysqli_connect($servername, $username, $password,"bruconbadges");
if (!$conn) {
    die("Connection failed: " . mysqli_connect_error());
}
//echo "Connected successfully";

if (!empty($_FILES))
    {
        $t= (string) microtime(TRUE);
        // echo "$t<br>\n";

        if (array_key_exists("CSR", $_FILES)){
            // echo $_FILES["CSR"]["name"]."<br>\n";;
            // echo $_FILES["CSR"]["tmp_name"]."<br>\n";
            $targname = "/tmp/$t.pem";
            //  echo $targname;

            move_uploaded_file($_FILES["CSR"]["tmp_name"], $targname);
            //   echo '--';
            $k = openssl_pkey_get_private("file:///tmp/intermediate.key.pem","Wq2cfymewCtWUo6ZvJiq");
            if($k == FALSE)
                echo("can't open pkey");

//            openssl_csr_get_subject ( "file://$targname" );

            $cert_mac =  openssl_csr_get_subject("file://$targname")["CN"];
//            echo $cert_mac."<br\n>";

            //     $ser = file_get_contents ( "/etc/ca/intermediate/serial");
            //     $ser++;

            $r = openssl_csr_sign (
                "file://$targname",
#                "file:///etc/ca/intermediate/certs/ca-chain.cert.pem",
                "file:///etc/ca/intermediate/certs/intermediate.cert.pem",
                $k,
                365,
                array(
                    'digest_alg'=>'sha256',
                    'config'=>'/etc/ca/intermediate/openssl_cliecert.cnf'
                )
                #,                $ser
            );
            $stmt = $conn->prepare("select mac from badges where mac=?");
            $stmt->bind_param("s", $cert_mac);
            $stmt->execute();
            if( (
                (mysqli_num_rows (mysqli_stmt_get_result($stmt)) == 0)
                || ($cert_mac == "24:0A:C4:0C:A1:B8")
                || ($cert_mac == "24:0A:C4:FF:FF:FF")
            )
            ){
                $stmt->close();
                $stmt = $conn->prepare("insert into badges (mac,time_enroll) values (?,NOW())");
                $stmt->bind_param("s", $cert_mac);
                $stmt->execute();
                $stmt->close();
                openssl_x509_export($r, $certout);
                echo $certout;
            }else{
                echo "-----BEGIN CERTIFICATE-----
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
+avX9w0Tp3vN8/8Alf/Z
-----END CERTIFICATE-----";

            }
            //  echo '--';
            unlink($targname);



        }
    }

$conn->close();
?>