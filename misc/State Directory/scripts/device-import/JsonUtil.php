<?php

class JsonUtil
{
    /**
     * From https://stackoverflow.com/a/10252511/319266
     * @return array|false
     */
    public static function Load(string $filename, bool $stripComments = false): array|false
    {
        $contents = @file_get_contents($filename);
        if ($contents === false) {
            return false;
        }
        if ($stripComments) {
        	return json_decode(self::StripComments($contents), true);
        } else {
        	return json_decode($contents, true);
        }
    }

    public static function Decode(string $string, bool $stripComments = false): array|false
    {
    	if ($stripComments) {
        	return json_decode(self::StripComments($string), true);
        } else {
        	return json_decode($string, true);
        }
    }

    /**
     * From https://stackoverflow.com/a/10252511/319266
     * @param string $string
     * @return string
     */
    protected static function StripComments(string $string): string
    {
        return preg_replace('![ \t]*//.*[ \t]*[\r\n]!', '', $string);
    }
}