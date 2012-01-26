#!/usr/local/bin/php
<?php
/****
*
* Utility to generate a CSV output from all language files to identify missing
* translation strings.
*
****/

// TODO: add footer for number of lines and completeness %

$language_path = './language'; // path for language files
$lang_files    = array();      // array containing languages and their respective strings.xml files
$lang          = array();      // array containing all language strings and their respective translations
/*
lang=array
(
	string_id => array
	{
		'language' => string,
		..
		..
	},
	..
	..
)
*/
define('EMPTY_STRING',   '""');
define('BASELINE_LANGUAGE', 'English');
define('DELIMITER', "\t");

if (is_dir($language_path))
{
	if ($dh = opendir($language_path))
	{
		while (false !== ($language_dir = readdir($dh)))
		{
			if (!in_array($language_dir, array('.', '..', constant('BASELINE_LANGUAGE'))) && is_dir("${language_path}/${language_dir}"))
			{
				$strings_file = "${language_path}/${language_dir}/strings.xml";
				if (is_readable($strings_file))
					$lang_files[$language_dir] = array
					(
						'file'          => $strings_file,
						'lines'         => 0,
						'missing_lines' => array(),
						'extra_lines'   => array(),
					);
			}
		}
		closedir($dh);
	}
	else
		die("unable to open $language_path\n");
}
else
	die("${language_path} is not a valid directory!\n");

ksort(&$lang_files); // sort language names
$languages = array_keys($lang_files);

// load baseline

$dummy = array();
foreach ($languages as $language)
	$dummy[$language] = constant('EMPTY_STRING');

if ($xml = simplexml_load_file($language_path . '/' . constant('BASELINE_LANGUAGE') . '/' . 'strings.xml'))
{
	foreach ($xml->string as $lang_string)
	{
		$string_id = sprintf('_%d', $lang_string['id']);
		$string    = '"' . preg_replace('/"/', "''", $lang_string[0]) . '"';
		$lang[$string_id] = array
		(
			constant('BASELINE_LANGUAGE') => $string,
		);
		$lang[$string_id] = array_merge($lang[$string_id], $dummy);
	}
}
else
	die("\n\n*** Failed to open baseline language! Check errors above, you probably have invalid XML entries like & in your strings file.\n");

$cnt = count($languages);
$i   = 0;
foreach ($languages as $language)
{
	//printf("Processing %s language file (%d of %d).\n", $language, ++$i, $cnt);
	if ($xml = simplexml_load_file($lang_files[$language]['file']))
	{
		foreach (array_keys($lang) as $string_id)
		{
			$id  = substr($string_id, 1);
			// yeah.. a non-optimal way to do it. much faster just to populate
                        // by comparing string ids. there were some issues there
                        // so moved to xpath instead.
			$str = $xml->xpath(sprintf('//string[@id=%s]', $id));
			if ($str)
			{
				$str = '"' . preg_replace('/"/', "''", $str[0]) . '"';
				$lang[$string_id][$language] = $str;
			}
			else
				array_push($lang_files[$language]['missing_lines'], $id);
		}
		$lang_files[$language]['lines']	= count($xml->string);
	}
	else
		die("Failed loading ${language} (${strings_file})!\n");
}

printf("string_id%s%s%s\n", constant('DELIMITER'), constant('BASELINE_LANGUAGE'), join(constant('DELIMITER'), $languages));

foreach ($lang as $string_id => $languages_strings)
	printf("%s%s%s\n", substr($string_id, 1), constant('DELIMITER'), join(constant('DELIMITER'), $languages_strings));
