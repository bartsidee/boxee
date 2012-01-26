<?php

$xml = simplexml_load_file("all.xml");
if (!$xml)
{
	echo "Cannot parse xml.\n";
	die;
}

$classes = array();

echo "<h1>Module <i>mc</i></h1>";
echo '<a href="#mc">Module functions and types</a><br/>';

foreach ($xml->xpath("/doxygen/compounddef/compoundname") as $classname)
{
	if (preg_match("/::(.+)/", $classname, $matches) == 0)
	{
		continue;
	}

	$classname = $matches[1];
	if ($classname == "AppException" || $classname == "ServerConfig")
	{
		continue;
	}

	if ($classname != "MC")
	{
	 echo '<a href="#'.$classname.'">Class: '.$classname.'</a><br/>';
	}
	array_push($classes, $classname);
}

echo "<hr/>";

echo "<h2><a name=\"mc\">Module functions and types</a></h2>";
$classXml = $xml->xpath("/doxygen/compounddef[compoundname='XAPP::MC']");
printMethods("MC", $xml);

foreach ($classes as $classname)
{
	if ($classname == "MC")
	{
		continue;
	}
	
	echo "<h2><a name=\"".$classname."\">Class: ".$classname."</a></h2>";
	
	$classXml = $xml->xpath("/doxygen/compounddef[compoundname='XAPP::".$classname."']");
	echo $classXml[0]->briefdescription->para;
	if ($classXml[0]->detaileddescription->para)
	{
    echo $classXml[0]->detaileddescription->para;
	}
	
	printMethods($classname, $xml);
}

function printMethods($classname, $xml)
{
  foreach ($xml->xpath("/doxygen/compounddef[compoundname='XAPP::".$classname."']") as $x)
  {
    if (!$x->xpath("sectiondef/memberdef[@prot='public']")) continue;

    foreach ($x->xpath("sectiondef/memberdef[@prot='public']") as $y)
    {
      $type = $y->type;
      if ($type->ref)
      {
        $type = $type->ref;
      }
      $name = $y->name;
      $args = $y->argsstring;
  
      $type = str_replace("&", "", $type);
      $type = str_replace("const ", "", $type);
      $type = str_replace("std::", "", $type);
      
      $linkType = "";
      if (strpos($type, "XAPP::") !== FALSE || ($type != "int" && $type != "void" && $type != "string" && $type != "bool"))
      {
        $type = str_replace("XAPP::", "", $type);
      	$linkType = "#".$type;
      }

      $args = str_replace("&", "", $args);
      $args = str_replace("const ", "", $args);
      $args = str_replace("std::", "", $args);
      $args = str_replace("XAPP::", "", $args);

      if ($y["kind"] == "enum")
      {
        $name = "enum ".$name;
      }
      
      echo "<h3>".$name."</h3>\n";
      
      if ($y["kind"] != "enum")
      {
        echo "<pre>";
        if ($linkType != "")
        {
          echo "<a href=\"".$linkType."\">";
        }
        echo trim($type);
        if ($linkType != "")
        {
        	echo "</a>";
        }
        echo " ".trim($name).$args."</pre>\n";
      }
      
      echo "<p>";
      if ($y->briefdescription->para)
      {
        echo $y->briefdescription->para;
      }
      if ($y->detaileddescription->para)
      {
        echo $y->detaileddescription->para;
      }
      echo "</p>";
      
      $i = 0;
      foreach ($y->xpath("detaileddescription/para/parameterlist/parameteritem/parameternamelist/parametername|detaileddescription/para/parameterlist/parameteritem/parameterdescription/para") as $elem => $value)
      {      
        if ($i % 2 == 0)
        {
          echo "<dl>\n";
          echo "<dt>".$value."</dt>";
        }
        else if ($i % 2 == 1)
        {
          echo "<dd>".$value."</dd>\n";
          echo "</dl>\n";
        }
        $i++;
      }
      
      foreach ($y->xpath("enumvalue") as $value)
      {
          echo "<dl>\n";
          echo "<dt>".$value->name."</dt>";
          echo "<dd>".$value->briefdescription->para." ";
          if ($value->detaileddescription->para)
          { 
            echo $value->detaileddescription->para;
          }
          echo "</dd>\n";
          echo "</dl>\n";
      }
    }   
    echo "<hr/>";
  }	
}

?>
