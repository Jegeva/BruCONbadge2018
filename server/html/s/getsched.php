<?php

header('Content-Type: application/json; charset: UTF-8');

$file_stamp = '/tmp/lastopen_brucon';
$file_jsonraw = '/tmp/sched_brucon';
$file_jsonfiltered = '/tmp/sched_filtered_brucon';
function getjson() {
     $opts = array('http'=>array('header' => "User-Agent:MyAgent/1.0\r\n"));
     $context = stream_context_create($opts);
     $url = "https://brucon0x0a.sched.com/api/session/export?api_key=YOUR_API_KEY&format=json&strip_html=Y";
     return file_get_contents($url,false,$context);
     
}

function jsonclean($json_raw){
         setlocale(LC_CTYPE, 'POSIX');
         $ar = json_decode( $json_raw);
       //  echo var_dump($ar);
         $retar = array(
          array(),
          array(),
          array()
         );
         foreach ($ar as $entry){
         $obj = (object)[];
         if(! property_exists($entry,"address")){
	         $entry->address = "?";
         }
         $obj->title = $entry->event_start_time."-".$entry->event_end_time." ".$entry->name;
         $obj->loc =   preg_replace("/^[0-9]+\.? */","",$entry->venue)."@".$entry->address;
         if(property_exists($entry,"speakers")){
              $obj->spk = "";
              foreach($entry->speakers as $spk){
               $obj->spk .= $spk->name.", " ;
              }
              $obj->spk = substr( $obj->spk,0,strlen( $obj->spk)-2);
         }

         $obj->evt =    substr($entry->event_type,0,1);    
                 switch($entry->event_start_day){
                 case "3":
              //   echo "3\n";
                 array_push($retar[0],$obj); 
                 break;
                 case "4":
               //  echo "4\n";
                 array_push($retar[1],$obj);
                 break;
                 case "5":
              //   echo "5\n";
                 array_push($retar[2],$obj);
                 break;
                 }
         }
         $json = json_encode($retar);
         $json=str_replace("\u201c",'\"',$json);
         $json=str_replace("\u201d",'\"',$json);
         return $json;
}

if(! file_exists($file_stamp)){
     $handle_stamp = fopen($file_stamp, 'x')
     or die('Cannot create file:  '.$file_stamp);

     fwrite($handle_stamp, time());

     $handle_jsonraw = fopen($file_jsonraw, 'x')
     or die('Cannot create file:  '.$file_jsonraw);

     $json = getjson();
     fwrite($handle_jsonraw, $json);
     
}else{

    $handle_stamp = fopen($file_stamp, 'r+')
         or die('Cannot create file:  '.$file_stamp);
    $time_lastopen = fread($handle_stamp,filesize($file_stamp));

    if( (time() - ((int)$time_lastopen)) > 120){
        $handle_jsonraw = fopen($file_jsonraw, 'w+');
        $json =  getjson(); 
        fwrite($handle_jsonraw, $json);
        fseek($handle_stamp,0);
        fwrite($handle_stamp, time());
    }else{
         $handle_jsonraw = fopen($file_jsonraw, 'r');
         $json = fread($handle_jsonraw,filesize($file_jsonraw));
    }

}

echo jsonclean($json);
?>
