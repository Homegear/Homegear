<?php
function sliceString($string, $maxCharacters)
{
	$i = 0;
	while (strlen($string) > $maxCharacters)
	{
		$temp = substr($string, 0, $maxCharacters);
		$pos = strrpos($temp, "+");
		$strings[$i] = substr($string, 0, $pos);
		$string = substr($string, $pos);
		$i++;
	}
	$strings[$i] = $string;
	return $strings;
}

if($argc != 3) die("Wrong parameter count. Please provice the language as first and the string to say as second parameter. E. g.: GoogleTTS.php de \"Hello World\"");

$language = $argv[1];

$path = "/var/lib/homegear/tmp/sonos/";
if(!file_exists($path))
{
	if(!mkdir($path, 0777, true)) die("Could not create directory $path");
}

$words = urlencode($argv[2]);
$filename = $path.md5($words)."-".$language.".mp3";

if(!file_exists($filename))
{
	$words = sliceString($words, 100);
	$data = array();
	for ($i = 0; $i < count($words); $i++)
	{
		$data[$i] = file_get_contents('http://translate.google.com/translate_tts?q='.$words[$i].'&tl='.$language);
	}
	file_put_contents($filename, $data);
}

echo $filename;
?>
