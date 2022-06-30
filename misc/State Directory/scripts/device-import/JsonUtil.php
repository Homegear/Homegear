<?php

class JsonUtil
{
    /**
     * From https://stackoverflow.com/a/10252511/319266
     * @return array|false
     */
    public static function Load(string $filename): array|false
    {
        $contents = @file_get_contents($filename);
        if ($contents === false) {
            return false;
        }
        return json_decode(self::StripComments($contents), true);
    }

    public static function Decode(string $string): string
    {
        return json_decode(self::StripComments($string), true);
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