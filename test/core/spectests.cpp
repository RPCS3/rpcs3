#include "spectests.h"
#include "handlermacros.h"
#include "specexamples.h"
#include "yaml-cpp/yaml.h"
#include <cassert>

namespace Test {
	namespace Spec {
		// 2.1
		TEST SeqScalars()
        {
            HANDLE(ex2_1);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SCALAR("?", 0, "Ken Griffey");
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.2
		TEST MappingScalarsToScalars()
        {
            HANDLE(ex2_2);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "hr");
            EXPECT_SCALAR("?", 0, "65");
            EXPECT_SCALAR("?", 0, "avg");
            EXPECT_SCALAR("?", 0, "0.278");
            EXPECT_SCALAR("?", 0, "rbi");
            EXPECT_SCALAR("?", 0, "147");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.3
		TEST MappingScalarsToSequences()
        {
            HANDLE(ex2_3);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "american");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Boston Red Sox");
            EXPECT_SCALAR("?", 0, "Detroit Tigers");
            EXPECT_SCALAR("?", 0, "New York Yankees");
            EXPECT_SEQ_END();
            EXPECT_SCALAR("?", 0, "national");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "New York Mets");
            EXPECT_SCALAR("?", 0, "Chicago Cubs");
            EXPECT_SCALAR("?", 0, "Atlanta Braves");
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.4
		TEST SequenceOfMappings()
        {
            HANDLE(ex2_4);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "name");
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_SCALAR("?", 0, "hr");
            EXPECT_SCALAR("?", 0, "65");
            EXPECT_SCALAR("?", 0, "avg");
            EXPECT_SCALAR("?", 0, "0.278");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "name");
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SCALAR("?", 0, "hr");
            EXPECT_SCALAR("?", 0, "63");
            EXPECT_SCALAR("?", 0, "avg");
            EXPECT_SCALAR("?", 0, "0.288");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.5
		TEST SequenceOfSequences()
        {
            HANDLE(ex2_5);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "name");
            EXPECT_SCALAR("?", 0, "hr");
            EXPECT_SCALAR("?", 0, "avg");
            EXPECT_SEQ_END();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_SCALAR("?", 0, "65");
            EXPECT_SCALAR("?", 0, "0.278");
            EXPECT_SEQ_END();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SCALAR("?", 0, "63");
            EXPECT_SCALAR("?", 0, "0.288");
            EXPECT_SEQ_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.6
		TEST MappingOfMappings()
        {
            HANDLE(ex2_6);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "hr");
            EXPECT_SCALAR("?", 0, "65");
            EXPECT_SCALAR("?", 0, "avg");
            EXPECT_SCALAR("?", 0, "0.278");
            EXPECT_MAP_END();
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "hr");
            EXPECT_SCALAR("?", 0, "63");
            EXPECT_SCALAR("?", 0, "avg");
            EXPECT_SCALAR("?", 0, "0.288");
            EXPECT_MAP_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.7
		TEST TwoDocumentsInAStream()
        {
            HANDLE(ex2_7);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SCALAR("?", 0, "Ken Griffey");
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Chicago Cubs");
            EXPECT_SCALAR("?", 0, "St Louis Cardinals");
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
        
		// 2.8
		TEST PlayByPlayFeed()
        {
            HANDLE(ex2_8);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "time");
            EXPECT_SCALAR("?", 0, "20:03:20");
            EXPECT_SCALAR("?", 0, "player");
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SCALAR("?", 0, "action");
            EXPECT_SCALAR("?", 0, "strike (miss)");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "time");
            EXPECT_SCALAR("?", 0, "20:03:47");
            EXPECT_SCALAR("?", 0, "player");
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SCALAR("?", 0, "action");
            EXPECT_SCALAR("?", 0, "grand slam");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.9
		TEST SingleDocumentWithTwoComments()
        {
            HANDLE(ex2_9);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "hr");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SEQ_END();
            EXPECT_SCALAR("?", 0, "rbi");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SCALAR("?", 0, "Ken Griffey");
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.10
		TEST SimpleAnchor()
        {
            HANDLE(ex2_10);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "hr");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_SCALAR("?", 1, "Sammy Sosa");
            EXPECT_SEQ_END();
            EXPECT_SCALAR("?", 0, "rbi");
            EXPECT_SEQ_START("?", 0);
            EXPECT_ALIAS(1);
            EXPECT_SCALAR("?", 0, "Ken Griffey");
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.11
		TEST MappingBetweenSequences()
        {
            HANDLE(ex2_11);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Detroit Tigers");
            EXPECT_SCALAR("?", 0, "Chicago cubs");
            EXPECT_SEQ_END();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "2001-07-23");
            EXPECT_SEQ_END();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "New York Yankees");
            EXPECT_SCALAR("?", 0, "Atlanta Braves");
            EXPECT_SEQ_END();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "2001-07-02");
            EXPECT_SCALAR("?", 0, "2001-08-12");
            EXPECT_SCALAR("?", 0, "2001-08-14");
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.12
		TEST CompactNestedMapping()
        {
            HANDLE(ex2_12);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "item");
            EXPECT_SCALAR("?", 0, "Super Hoop");
            EXPECT_SCALAR("?", 0, "quantity");
            EXPECT_SCALAR("?", 0, "1");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "item");
            EXPECT_SCALAR("?", 0, "Basketball");
            EXPECT_SCALAR("?", 0, "quantity");
            EXPECT_SCALAR("?", 0, "4");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "item");
            EXPECT_SCALAR("?", 0, "Big Shoes");
            EXPECT_SCALAR("?", 0, "quantity");
            EXPECT_SCALAR("?", 0, "1");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.13
		TEST InLiteralsNewlinesArePreserved()
        {
            HANDLE(ex2_13);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0,
                          "\\//||\\/||\n"
                          "// ||  ||__");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.14
		TEST InFoldedScalarsNewlinesBecomeSpaces()
        {
            HANDLE(ex2_14);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, "Mark McGwire's year was crippled by a knee injury.");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.15
		TEST FoldedNewlinesArePreservedForMoreIndentedAndBlankLines()
        {
            HANDLE(ex2_15);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0,
                          "Sammy Sosa completed another fine season with great stats.\n"
                          "\n"
                          "  63 Home Runs\n"
                          "  0.288 Batting Average\n"
                          "\n"
                          "What a year!");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.16
		TEST IndentationDeterminesScope()
        {
            HANDLE(ex2_16);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "name");
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_SCALAR("?", 0, "accomplishment");
            EXPECT_SCALAR("!", 0, "Mark set a major league home run record in 1998.\n");
            EXPECT_SCALAR("?", 0, "stats");
            EXPECT_SCALAR("!", 0,
                          "65 Home Runs\n"
                          "0.278 Batting Average\n");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.17
		TEST QuotedScalars()
        {
            HANDLE(ex2_17);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "unicode");
            EXPECT_SCALAR("!", 0, "Sosa did fine.\u263A");
            EXPECT_SCALAR("?", 0, "control");
            EXPECT_SCALAR("!", 0, "\b1998\t1999\t2000\n");
            EXPECT_SCALAR("?", 0, "hex esc");
            EXPECT_SCALAR("!", 0, "\x0d\x0a is \r\n");
            EXPECT_SCALAR("?", 0, "single");
            EXPECT_SCALAR("!", 0, "\"Howdy!\" he cried.");
            EXPECT_SCALAR("?", 0, "quoted");
            EXPECT_SCALAR("!", 0, " # Not a 'comment'.");
            EXPECT_SCALAR("?", 0, "tie-fighter");
            EXPECT_SCALAR("!", 0, "|\\-*-/|");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.18
		TEST MultiLineFlowScalars()
        {
            HANDLE(ex2_18);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "plain");
            EXPECT_SCALAR("?", 0, "This unquoted scalar spans many lines.");
            EXPECT_SCALAR("?", 0, "quoted");
            EXPECT_SCALAR("!", 0, "So does this quoted scalar.\n");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// TODO: 2.19 - 2.22 schema tags
		
		// 2.23
		TEST VariousExplicitTags()
        {
            HANDLE(ex2_23);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "not-date");
            EXPECT_SCALAR("tag:yaml.org,2002:str", 0, "2002-04-28");
            EXPECT_SCALAR("?", 0, "picture");
            EXPECT_SCALAR("tag:yaml.org,2002:binary", 0,
                          "R0lGODlhDAAMAIQAAP//9/X\n"
                          "17unp5WZmZgAAAOfn515eXv\n"
                          "Pz7Y6OjuDg4J+fn5OTk6enp\n"
                          "56enmleECcgggoBADs=\n");
            EXPECT_SCALAR("?", 0, "application specific tag");
            EXPECT_SCALAR("!something", 0,
                          "The semantics of the tag\n"
                          "above may be different for\n"
                          "different documents.");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.24
		TEST GlobalTags()
        {
            HANDLE(ex2_24);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("tag:clarkevans.com,2002:shape", 0);
            EXPECT_MAP_START("tag:clarkevans.com,2002:circle", 0);
            EXPECT_SCALAR("?", 0, "center");
            EXPECT_MAP_START("?", 1);
            EXPECT_SCALAR("?", 0, "x");
            EXPECT_SCALAR("?", 0, "73");
            EXPECT_SCALAR("?", 0, "y");
            EXPECT_SCALAR("?", 0, "129");
            EXPECT_MAP_END();
            EXPECT_SCALAR("?", 0, "radius");
            EXPECT_SCALAR("?", 0, "7");
            EXPECT_MAP_END();
            EXPECT_MAP_START("tag:clarkevans.com,2002:line", 0);
            EXPECT_SCALAR("?", 0, "start");
            EXPECT_ALIAS(1);
            EXPECT_SCALAR("?", 0, "finish");
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "x");
            EXPECT_SCALAR("?", 0, "89");
            EXPECT_SCALAR("?", 0, "y");
            EXPECT_SCALAR("?", 0, "102");
            EXPECT_MAP_END();
            EXPECT_MAP_END();
            EXPECT_MAP_START("tag:clarkevans.com,2002:label", 0);
            EXPECT_SCALAR("?", 0, "start");
            EXPECT_ALIAS(1);
            EXPECT_SCALAR("?", 0, "color");
            EXPECT_SCALAR("?", 0, "0xFFEEBB");
            EXPECT_SCALAR("?", 0, "text");
            EXPECT_SCALAR("?", 0, "Pretty vector drawing.");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.25
		TEST UnorderedSets()
        {
            HANDLE(ex2_25);
            EXPECT_DOC_START();
            EXPECT_MAP_START("tag:yaml.org,2002:set", 0);
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_NULL(0);
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_NULL(0);
            EXPECT_SCALAR("?", 0, "Ken Griffey");
            EXPECT_NULL(0);
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.26
		TEST OrderedMappings()
        {
            HANDLE(ex2_26);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("tag:yaml.org,2002:omap", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_SCALAR("?", 0, "65");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SCALAR("?", 0, "63");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "Ken Griffey");
            EXPECT_SCALAR("?", 0, "58");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.27
		TEST Invoice()
        {
            HANDLE(ex2_27);
            EXPECT_DOC_START();
            EXPECT_MAP_START("tag:clarkevans.com,2002:invoice", 0);
            EXPECT_SCALAR("?", 0, "invoice");
            EXPECT_SCALAR("?", 0, "34843");
            EXPECT_SCALAR("?", 0, "date");
            EXPECT_SCALAR("?", 0, "2001-01-23");
            EXPECT_SCALAR("?", 0, "bill-to");
            EXPECT_MAP_START("?", 1);
            EXPECT_SCALAR("?", 0, "given");
            EXPECT_SCALAR("?", 0, "Chris");
            EXPECT_SCALAR("?", 0, "family");
            EXPECT_SCALAR("?", 0, "Dumars");
            EXPECT_SCALAR("?", 0, "address");
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "lines");
            EXPECT_SCALAR("!", 0,
                          "458 Walkman Dr.\n"
                          "Suite #292\n");
            EXPECT_SCALAR("?", 0, "city");
            EXPECT_SCALAR("?", 0, "Royal Oak");
            EXPECT_SCALAR("?", 0, "state");
            EXPECT_SCALAR("?", 0, "MI");
            EXPECT_SCALAR("?", 0, "postal");
            EXPECT_SCALAR("?", 0, "48046");
            EXPECT_MAP_END();
            EXPECT_MAP_END();
            EXPECT_SCALAR("?", 0, "ship-to");
            EXPECT_ALIAS(1);
            EXPECT_SCALAR("?", 0, "product");
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "sku");
            EXPECT_SCALAR("?", 0, "BL394D");
            EXPECT_SCALAR("?", 0, "quantity");
            EXPECT_SCALAR("?", 0, "4");
            EXPECT_SCALAR("?", 0, "description");
            EXPECT_SCALAR("?", 0, "Basketball");
            EXPECT_SCALAR("?", 0, "price");
            EXPECT_SCALAR("?", 0, "450.00");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "sku");
            EXPECT_SCALAR("?", 0, "BL4438H");
            EXPECT_SCALAR("?", 0, "quantity");
            EXPECT_SCALAR("?", 0, "1");
            EXPECT_SCALAR("?", 0, "description");
            EXPECT_SCALAR("?", 0, "Super Hoop");
            EXPECT_SCALAR("?", 0, "price");
            EXPECT_SCALAR("?", 0, "2392.00");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_SCALAR("?", 0, "tax");
            EXPECT_SCALAR("?", 0, "251.42");
            EXPECT_SCALAR("?", 0, "total");
            EXPECT_SCALAR("?", 0, "4443.52");
            EXPECT_SCALAR("?", 0, "comments");
            EXPECT_SCALAR("?", 0, "Late afternoon is best. Backup contact is Nancy Billsmer @ 338-4338.");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.28
		TEST LogFile()
        {
            HANDLE(ex2_28);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "Time");
            EXPECT_SCALAR("?", 0, "2001-11-23 15:01:42 -5");
            EXPECT_SCALAR("?", 0, "User");
            EXPECT_SCALAR("?", 0, "ed");
            EXPECT_SCALAR("?", 0, "Warning");
            EXPECT_SCALAR("?", 0, "This is an error message for the log file");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "Time");
            EXPECT_SCALAR("?", 0, "2001-11-23 15:02:31 -5");
            EXPECT_SCALAR("?", 0, "User");
            EXPECT_SCALAR("?", 0, "ed");
            EXPECT_SCALAR("?", 0, "Warning");
            EXPECT_SCALAR("?", 0, "A slightly different error message.");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "Date");
            EXPECT_SCALAR("?", 0, "2001-11-23 15:03:17 -5");
            EXPECT_SCALAR("?", 0, "User");
            EXPECT_SCALAR("?", 0, "ed");
            EXPECT_SCALAR("?", 0, "Fatal");
            EXPECT_SCALAR("?", 0, "Unknown variable \"bar\"");
            EXPECT_SCALAR("?", 0, "Stack");
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "file");
            EXPECT_SCALAR("?", 0, "TopClass.py");
            EXPECT_SCALAR("?", 0, "line");
            EXPECT_SCALAR("?", 0, "23");
            EXPECT_SCALAR("?", 0, "code");
            EXPECT_SCALAR("!", 0, "x = MoreObject(\"345\\n\")\n");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "file");
            EXPECT_SCALAR("?", 0, "MoreClass.py");
            EXPECT_SCALAR("?", 0, "line");
            EXPECT_SCALAR("?", 0, "58");
            EXPECT_SCALAR("?", 0, "code");
            EXPECT_SCALAR("!", 0, "foo = bar");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// TODO: 5.1 - 5.2 BOM
		
		// 5.3
		TEST BlockStructureIndicators()
        {
            HANDLE(ex5_3);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "sequence");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "one");
            EXPECT_SCALAR("?", 0, "two");
            EXPECT_SEQ_END();
            EXPECT_SCALAR("?", 0, "mapping");
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "sky");
            EXPECT_SCALAR("?", 0, "blue");
            EXPECT_SCALAR("?", 0, "sea");
            EXPECT_SCALAR("?", 0, "green");
            EXPECT_MAP_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 5.4
		TEST FlowStructureIndicators()
        {
            HANDLE(ex5_4);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "sequence");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "one");
            EXPECT_SCALAR("?", 0, "two");
            EXPECT_SEQ_END();
            EXPECT_SCALAR("?", 0, "mapping");
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "sky");
            EXPECT_SCALAR("?", 0, "blue");
            EXPECT_SCALAR("?", 0, "sea");
            EXPECT_SCALAR("?", 0, "green");
            EXPECT_MAP_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 5.5
		TEST CommentIndicator()
        {
            HANDLE(ex5_5);
            DONE();
        }
		
		// 5.6
		TEST NodePropertyIndicators()
        {
            HANDLE(ex5_6);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "anchored");
            EXPECT_SCALAR("!local", 1, "value");
            EXPECT_SCALAR("?", 0, "alias");
            EXPECT_ALIAS(1);
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 5.7
		TEST BlockScalarIndicators()
        {
            HANDLE(ex5_7);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "literal");
            EXPECT_SCALAR("!", 0, "some\ntext\n");
            EXPECT_SCALAR("?", 0, "folded");
            EXPECT_SCALAR("!", 0, "some text\n");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 5.8
		TEST QuotedScalarIndicators()
        {
            HANDLE(ex5_8);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "single");
            EXPECT_SCALAR("!", 0, "text");
            EXPECT_SCALAR("?", 0, "double");
            EXPECT_SCALAR("!", 0, "text");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// TODO: 5.9 directive
		// TODO: 5.10 reserved indicator
		
		// 5.11
		TEST LineBreakCharacters()
        {
            HANDLE(ex5_11);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0,
                          "Line break (no glyph)\n"
                          "Line break (glyphed)\n");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 5.12
		TEST TabsAndSpaces()
        {
            HANDLE(ex5_12);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "quoted");
            EXPECT_SCALAR("!", 0, "Quoted\t");
            EXPECT_SCALAR("?", 0, "block");
            EXPECT_SCALAR("!", 0,
                          "void main() {\n"
                          "\tprintf(\"Hello, world!\\n\");\n"
                          "}");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 5.13
		TEST EscapedCharacters()
        {
            HANDLE(ex5_13);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, "Fun with \x5C \x22 \x07 \x08 \x1B \x0C \x0A \x0D \x09 \x0B " + std::string("\x00", 1) + " \x20 \xA0 \x85 \xe2\x80\xa8 \xe2\x80\xa9 A A A");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 5.14
		TEST InvalidEscapedCharacters()
        {
            try {
                HANDLE(ex5_14);
            } catch(const YAML::ParserException& e) {
				YAML_ASSERT(e.msg == std::string(YAML::ErrorMsg::INVALID_ESCAPE) + "c");
				return true;
			}
            return "  no exception caught";
        }
		
		// 6.1
		TEST IndentationSpaces()
        {
            HANDLE(ex6_1);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "Not indented");
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "By one space");
            EXPECT_SCALAR("!", 0, "By four\n  spaces\n");
            EXPECT_SCALAR("?", 0, "Flow style");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "By two");
            EXPECT_SCALAR("?", 0, "Also by two");
            EXPECT_SCALAR("?", 0, "Still by two");
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.2
		TEST IndentationIndicators()
        {
            HANDLE(ex6_2);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "a");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "b");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "c");
            EXPECT_SCALAR("?", 0, "d");
            EXPECT_SEQ_END();
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.3
		TEST SeparationSpaces()
        {
            HANDLE(ex6_3);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "foo");
            EXPECT_SCALAR("?", 0, "bar");
            EXPECT_MAP_END();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "baz");
            EXPECT_SCALAR("?", 0, "baz");
            EXPECT_SEQ_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.4
		TEST LinePrefixes()
        {
            HANDLE(ex6_4);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "plain");
            EXPECT_SCALAR("?", 0, "text lines");
            EXPECT_SCALAR("?", 0, "quoted");
            EXPECT_SCALAR("!", 0, "text lines");
            EXPECT_SCALAR("?", 0, "block");
            EXPECT_SCALAR("!", 0, "text\n \tlines\n");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.5
		TEST EmptyLines()
        {
            HANDLE(ex6_5);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "Folding");
            EXPECT_SCALAR("!", 0, "Empty line\nas a line feed");
            EXPECT_SCALAR("?", 0, "Chomping");
            EXPECT_SCALAR("!", 0, "Clipped empty lines\n");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.6
		TEST LineFolding()
        {
            HANDLE(ex6_6);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, "trimmed\n\n\nas space");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.7
		TEST BlockFolding()
        {
            HANDLE(ex6_7);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, "foo \n\n\t bar\n\nbaz\n");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.8
		TEST FlowFolding()
        {
            HANDLE(ex6_8);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, " foo\nbar\nbaz ");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.9
		TEST SeparatedComment()
        {
            HANDLE(ex6_9);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "key");
            EXPECT_SCALAR("?", 0, "value");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.10
		TEST CommentLines()
        {
            HANDLE(ex6_10);
            DONE();
        }
		
		// 6.11
		TEST MultiLineComments()
        {
            HANDLE(ex6_11);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "key");
            EXPECT_SCALAR("?", 0, "value");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
        
		// 6.12
		TEST SeparationSpacesII()
        {
            HANDLE(ex6_12);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "first");
            EXPECT_SCALAR("?", 0, "Sammy");
            EXPECT_SCALAR("?", 0, "last");
            EXPECT_SCALAR("?", 0, "Sosa");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "hr");
            EXPECT_SCALAR("?", 0, "65");
            EXPECT_SCALAR("?", 0, "avg");
            EXPECT_SCALAR("?", 0, "0.278");
            EXPECT_MAP_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.13
		TEST ReservedDirectives()
        {
            HANDLE(ex6_13);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, "foo");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.14
		TEST YAMLDirective()
        {
            HANDLE(ex6_14);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, "foo");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.15
		TEST InvalidRepeatedYAMLDirective()
        {
            try {
                HANDLE(ex6_15);
			} catch(const YAML::ParserException& e) {
				if(e.msg == YAML::ErrorMsg::REPEATED_YAML_DIRECTIVE)
					return true;
                
				throw;
			}
			
			return "  No exception was thrown";
        }
		
		// 6.16
		TEST TagDirective()
        {
            HANDLE(ex6_16);
            EXPECT_DOC_START();
            EXPECT_SCALAR("tag:yaml.org,2002:str", 0, "foo");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.17
		TEST InvalidRepeatedTagDirective()
        {
            try {
                HANDLE(ex6_17);
			} catch(const YAML::ParserException& e) {
				if(e.msg == YAML::ErrorMsg::REPEATED_TAG_DIRECTIVE)
					return true;
				
				throw;
			}
            
			return "  No exception was thrown";
        }
		
		// 6.18
		TEST PrimaryTagHandle()
        {
            HANDLE(ex6_18);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!foo", 0, "bar");
            EXPECT_DOC_END();
            EXPECT_DOC_START();
            EXPECT_SCALAR("tag:example.com,2000:app/foo", 0, "bar");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.19
		TEST SecondaryTagHandle()
        {
            HANDLE(ex6_19);
            EXPECT_DOC_START();
            EXPECT_SCALAR("tag:example.com,2000:app/int", 0, "1 - 3");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.20
		TEST TagHandles()
        {
            HANDLE(ex6_20);
            EXPECT_DOC_START();
            EXPECT_SCALAR("tag:example.com,2000:app/foo", 0, "bar");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.21
		TEST LocalTagPrefix()
        {
            HANDLE(ex6_21);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!my-light", 0, "fluorescent");
            EXPECT_DOC_END();
            EXPECT_DOC_START();
            EXPECT_SCALAR("!my-light", 0, "green");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.22
		TEST GlobalTagPrefix()
        {
            HANDLE(ex6_22);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("tag:example.com,2000:app/foo", 0, "bar");
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.23
		TEST NodeProperties()
        {
            HANDLE(ex6_23);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("tag:yaml.org,2002:str", 1, "foo");
            EXPECT_SCALAR("tag:yaml.org,2002:str", 0, "bar");
            EXPECT_SCALAR("?", 2, "baz");
            EXPECT_ALIAS(1);
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.24
		TEST VerbatimTags()
        {
            HANDLE(ex6_24);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("tag:yaml.org,2002:str", 0, "foo");
            EXPECT_SCALAR("!bar", 0, "baz");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.25
		TEST InvalidVerbatimTags()
        {
            HANDLE(ex6_25);
            return "  not implemented yet";
        }
		
		// 6.26
		TEST TagShorthands()
        {
            HANDLE(ex6_26);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("!local", 0, "foo");
            EXPECT_SCALAR("tag:yaml.org,2002:str", 0, "bar");
            EXPECT_SCALAR("tag:example.com,2000:app/tag%21", 0, "baz");
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.27
		TEST InvalidTagShorthands()
        {
			bool threw = false;
			try {
				HANDLE(ex6_27a);
			} catch(const YAML::ParserException& e) {
				threw = true;
				if(e.msg != YAML::ErrorMsg::TAG_WITH_NO_SUFFIX)
					throw;
			}
			
			if(!threw)
				return "  No exception was thrown for a tag with no suffix";
            
			HANDLE(ex6_27b); // TODO: should we reject this one (since !h! is not declared)?
			return "  not implemented yet";
        }
		
		// 6.28
		TEST NonSpecificTags()
        {
            HANDLE(ex6_28);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("!", 0, "12");
            EXPECT_SCALAR("?", 0, "12");
            EXPECT_SCALAR("!", 0, "12");
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 6.29
		TEST NodeAnchors()
        {
            HANDLE(ex6_29);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "First occurrence");
            EXPECT_SCALAR("?", 1, "Value");
            EXPECT_SCALAR("?", 0, "Second occurrence");
            EXPECT_ALIAS(1);
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.1
		TEST AliasNodes()
        {
            HANDLE(ex7_1);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "First occurrence");
            EXPECT_SCALAR("?", 1, "Foo");
            EXPECT_SCALAR("?", 0, "Second occurrence");
            EXPECT_ALIAS(1);
            EXPECT_SCALAR("?", 0, "Override anchor");
            EXPECT_SCALAR("?", 2, "Bar");
            EXPECT_SCALAR("?", 0, "Reuse anchor");
            EXPECT_ALIAS(2);
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.2
		TEST EmptyNodes()
        {
            HANDLE(ex7_2);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "foo");
            EXPECT_SCALAR("tag:yaml.org,2002:str", 0, "");
            EXPECT_SCALAR("tag:yaml.org,2002:str", 0, "");
            EXPECT_SCALAR("?", 0, "bar");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.3
		TEST CompletelyEmptyNodes()
        {
            HANDLE(ex7_3);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "foo");
            EXPECT_NULL(0);
            EXPECT_NULL(0);
            EXPECT_SCALAR("?", 0, "bar");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.4
		TEST DoubleQuotedImplicitKeys()
        {
            HANDLE(ex7_4);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("!", 0, "implicit block key");
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("!", 0, "implicit flow key");
            EXPECT_SCALAR("?", 0, "value");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.5
		TEST DoubleQuotedLineBreaks()
        {
            HANDLE(ex7_5);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, "folded to a space,\nto a line feed, or \t \tnon-content");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.6
		TEST DoubleQuotedLines()
        {
            HANDLE(ex7_6);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, " 1st non-empty\n2nd non-empty 3rd non-empty ");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.7
		TEST SingleQuotedCharacters()
        {
            HANDLE(ex7_7);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, "here's to \"quotes\"");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.8
		TEST SingleQuotedImplicitKeys()
        {
            HANDLE(ex7_8);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("!", 0, "implicit block key");
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("!", 0, "implicit flow key");
            EXPECT_SCALAR("?", 0, "value");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.9
		TEST SingleQuotedLines()
        {
            HANDLE(ex7_9);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, " 1st non-empty\n2nd non-empty 3rd non-empty ");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.10
		TEST PlainCharacters()
        {
            HANDLE(ex7_10);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "::vector");
            EXPECT_SCALAR("!", 0, ": - ()");
            EXPECT_SCALAR("?", 0, "Up, up, and away!");
            EXPECT_SCALAR("?", 0, "-123");
            EXPECT_SCALAR("?", 0, "http://example.com/foo#bar");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "::vector");
            EXPECT_SCALAR("!", 0, ": - ()");
            EXPECT_SCALAR("!", 0, "Up, up, and away!");
            EXPECT_SCALAR("?", 0, "-123");
            EXPECT_SCALAR("?", 0, "http://example.com/foo#bar");
            EXPECT_SEQ_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.11
		TEST PlainImplicitKeys()
        {
            HANDLE(ex7_11);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "implicit block key");
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "implicit flow key");
            EXPECT_SCALAR("?", 0, "value");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.12
		TEST PlainLines()
        {
            HANDLE(ex7_12);
            EXPECT_DOC_START();
            EXPECT_SCALAR("?", 0, "1st non-empty\n2nd non-empty 3rd non-empty");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.13
		TEST FlowSequence()
        {
            HANDLE(ex7_13);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "one");
            EXPECT_SCALAR("?", 0, "two");
            EXPECT_SEQ_END();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "three");
            EXPECT_SCALAR("?", 0, "four");
            EXPECT_SEQ_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.14
		TEST FlowSequenceEntries()
        {
            HANDLE(ex7_14);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("!", 0, "double quoted");
            EXPECT_SCALAR("!", 0, "single quoted");
            EXPECT_SCALAR("?", 0, "plain text");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "nested");
            EXPECT_SEQ_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "single");
            EXPECT_SCALAR("?", 0, "pair");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.15
		TEST FlowMappings()
        {
            HANDLE(ex7_15);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "one");
            EXPECT_SCALAR("?", 0, "two");
            EXPECT_SCALAR("?", 0, "three");
            EXPECT_SCALAR("?", 0, "four");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "five");
            EXPECT_SCALAR("?", 0, "six");
            EXPECT_SCALAR("?", 0, "seven");
            EXPECT_SCALAR("?", 0, "eight");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.16
		TEST FlowMappingEntries()
        {
            HANDLE(ex7_16);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "explicit");
            EXPECT_SCALAR("?", 0, "entry");
            EXPECT_SCALAR("?", 0, "implicit");
            EXPECT_SCALAR("?", 0, "entry");
            EXPECT_NULL(0);
            EXPECT_NULL(0);
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.17
		TEST FlowMappingSeparateValues()
        {
            HANDLE(ex7_17);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "unquoted");
            EXPECT_SCALAR("!", 0, "separate");
            EXPECT_SCALAR("?", 0, "http://foo.com");
            EXPECT_NULL(0);
            EXPECT_SCALAR("?", 0, "omitted value");
            EXPECT_NULL(0);
            EXPECT_NULL(0);
            EXPECT_SCALAR("?", 0, "omitted key");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.18
		TEST FlowMappingAdjacentValues()
        {
            HANDLE(ex7_18);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("!", 0, "adjacent");
            EXPECT_SCALAR("?", 0, "value");
            EXPECT_SCALAR("!", 0, "readable");
            EXPECT_SCALAR("?", 0, "value");
            EXPECT_SCALAR("!", 0, "empty");
            EXPECT_NULL(0);
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.19
		TEST SinglePairFlowMappings()
        {
            HANDLE(ex7_19);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "foo");
            EXPECT_SCALAR("?", 0, "bar");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.20
		TEST SinglePairExplicitEntry()
        {
            HANDLE(ex7_20);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "foo bar");
            EXPECT_SCALAR("?", 0, "baz");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.21
		TEST SinglePairImplicitEntries()
        {
            HANDLE(ex7_21);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "YAML");
            EXPECT_SCALAR("?", 0, "separate");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_NULL(0);
            EXPECT_SCALAR("?", 0, "empty key entry");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "JSON");
            EXPECT_SCALAR("?", 0, "like");
            EXPECT_MAP_END();
            EXPECT_SCALAR("?", 0, "adjacent");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.22
		TEST InvalidImplicitKeys()
        {
            try {
                HANDLE(ex7_22);
            } catch(const YAML::Exception& e) {
				if(e.msg == YAML::ErrorMsg::END_OF_SEQ_FLOW)
					return true;
				
				throw;
			}
			return "  no exception thrown";
        }
		
		// 7.23
		TEST FlowContent()
        {
            HANDLE(ex7_23);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "a");
            EXPECT_SCALAR("?", 0, "b");
            EXPECT_SEQ_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "a");
            EXPECT_SCALAR("?", 0, "b");
            EXPECT_MAP_END();
            EXPECT_SCALAR("!", 0, "a");
            EXPECT_SCALAR("!", 0, "b");
            EXPECT_SCALAR("?", 0, "c");
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 7.24
		TEST FlowNodes()
        {
            HANDLE(ex7_24);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("tag:yaml.org,2002:str", 0, "a");
            EXPECT_SCALAR("!", 0, "b");
            EXPECT_SCALAR("!", 1, "c");
            EXPECT_ALIAS(1);
            EXPECT_SCALAR("tag:yaml.org,2002:str", 0, "");
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.1
		TEST BlockScalarHeader()
        {
            HANDLE(ex8_1);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("!", 0, "literal\n");
            EXPECT_SCALAR("!", 0, " folded\n");
            EXPECT_SCALAR("!", 0, "keep\n\n");
            EXPECT_SCALAR("!", 0, " strip");
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.2
		TEST BlockIndentationHeader()
        {
            HANDLE(ex8_2);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("!", 0, "detected\n");
            EXPECT_SCALAR("!", 0, "\n\n# detected\n");
            EXPECT_SCALAR("!", 0, " explicit\n");
            EXPECT_SCALAR("!", 0, "\t\ndetected\n");
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.3
		TEST InvalidBlockScalarIndentationIndicators()
        {
			{
				bool threw = false;
				try {
					HANDLE(ex8_3a);
				} catch(const YAML::Exception& e) {
					if(e.msg != YAML::ErrorMsg::END_OF_SEQ)
						throw;
					
					threw = true;
				}
				
				if(!threw)
					return "  no exception thrown for less indented auto-detecting indentation for a literal block scalar";
			}
			
			{
				bool threw = false;
				try {
					HANDLE(ex8_3b);
				} catch(const YAML::Exception& e) {
					if(e.msg != YAML::ErrorMsg::END_OF_SEQ)
						throw;
					
					threw = true;
				}
				
				if(!threw)
					return "  no exception thrown for less indented auto-detecting indentation for a folded block scalar";
			}
			
			{
				bool threw = false;
				try {
					HANDLE(ex8_3c);
				} catch(const YAML::Exception& e) {
					if(e.msg != YAML::ErrorMsg::END_OF_SEQ)
						throw;
					
					threw = true;
				}
				
				if(!threw)
					return "  no exception thrown for less indented explicit indentation for a literal block scalar";
			}
			
			return true;
        }
		
		// 8.4
		TEST ChompingFinalLineBreak()
        {
            HANDLE(ex8_4);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "strip");
            EXPECT_SCALAR("!", 0, "text");
            EXPECT_SCALAR("?", 0, "clip");
            EXPECT_SCALAR("!", 0, "text\n");
            EXPECT_SCALAR("?", 0, "keep");
            EXPECT_SCALAR("!", 0, "text\n");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.5
		TEST ChompingTrailingLines()
        {
            HANDLE(ex8_5);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "strip");
            EXPECT_SCALAR("!", 0, "# text");
            EXPECT_SCALAR("?", 0, "clip");
            EXPECT_SCALAR("!", 0, "# text\n");
            EXPECT_SCALAR("?", 0, "keep");
            EXPECT_SCALAR("!", 0, "# text\n"); // Note: I believe this is a bug in the YAML spec - it should be "# text\n\n"
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.6
		TEST EmptyScalarChomping()
        {
            HANDLE(ex8_6);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "strip");
            EXPECT_SCALAR("!", 0, "");
            EXPECT_SCALAR("?", 0, "clip");
            EXPECT_SCALAR("!", 0, "");
            EXPECT_SCALAR("?", 0, "keep");
            EXPECT_SCALAR("!", 0, "\n");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.7
		TEST LiteralScalar()
        {
            HANDLE(ex8_7);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, "literal\n\ttext\n");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.8
		TEST LiteralContent()
        {
            HANDLE(ex8_8);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, "\n\nliteral\n \n\ntext\n");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.9
		TEST FoldedScalar()
        {
            HANDLE(ex8_9);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, "folded text\n");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.10
		TEST FoldedLines()
        {
            HANDLE(ex8_10);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, "\nfolded line\nnext line\n  * bullet\n\n  * list\n  * lines\n\nlast line\n");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.11
		TEST MoreIndentedLines()
        {
            HANDLE(ex8_11);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, "\nfolded line\nnext line\n  * bullet\n\n  * list\n  * lines\n\nlast line\n");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.12
		TEST EmptySeparationLines()
        {
            HANDLE(ex8_12);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, "\nfolded line\nnext line\n  * bullet\n\n  * list\n  * lines\n\nlast line\n");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.13
		TEST FinalEmptyLines()
        {
            HANDLE(ex8_13);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, "\nfolded line\nnext line\n  * bullet\n\n  * list\n  * lines\n\nlast line\n");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.14
		TEST BlockSequence()
        {
            HANDLE(ex8_14);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "block sequence");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "one");
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "two");
            EXPECT_SCALAR("?", 0, "three");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.15
		TEST BlockSequenceEntryTypes()
        {
            HANDLE(ex8_15);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_NULL(0);
            EXPECT_SCALAR("!", 0, "block node\n");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "one");
            EXPECT_SCALAR("?", 0, "two");
            EXPECT_SEQ_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "one");
            EXPECT_SCALAR("?", 0, "two");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.16
		TEST BlockMappings()
        {
            HANDLE(ex8_16);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "block mapping");
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "key");
            EXPECT_SCALAR("?", 0, "value");
            EXPECT_MAP_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.17
		TEST ExplicitBlockMappingEntries()
        {
            HANDLE(ex8_17);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "explicit key");
            EXPECT_NULL(0);
            EXPECT_SCALAR("!", 0, "block key\n");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "one");
            EXPECT_SCALAR("?", 0, "two");
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.18
		TEST ImplicitBlockMappingEntries()
        {
            HANDLE(ex8_18);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "plain key");
            EXPECT_SCALAR("?", 0, "in-line value");
            EXPECT_NULL(0);
            EXPECT_NULL(0);
            EXPECT_SCALAR("!", 0, "quoted key");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "entry");
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.19
		TEST CompactBlockMappings()
        {
            HANDLE(ex8_19);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "sun");
            EXPECT_SCALAR("?", 0, "yellow");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "earth");
            EXPECT_SCALAR("?", 0, "blue");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "moon");
            EXPECT_SCALAR("?", 0, "white");
            EXPECT_MAP_END();
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.20
		TEST BlockNodeTypes()
        {
            HANDLE(ex8_20);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("!", 0, "flow in block");
            EXPECT_SCALAR("!", 0, "Block scalar\n");
            EXPECT_MAP_START("tag:yaml.org,2002:map", 0);
            EXPECT_SCALAR("?", 0, "foo");
            EXPECT_SCALAR("?", 0, "bar");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.21
		TEST BlockScalarNodes()
        {
            HANDLE(ex8_21);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "literal");
            EXPECT_SCALAR("!", 0, "value"); // Note: I believe this is a bug in the YAML spec - it should be "value\n"
            EXPECT_SCALAR("?", 0, "folded");
            EXPECT_SCALAR("!foo", 0, "value");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 8.22
		TEST BlockCollectionNodes()
        {
            HANDLE(ex8_22);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "sequence");
            EXPECT_SEQ_START("tag:yaml.org,2002:seq", 0);
            EXPECT_SCALAR("?", 0, "entry");
            EXPECT_SEQ_START("tag:yaml.org,2002:seq", 0);
            EXPECT_SCALAR("?", 0, "nested");
            EXPECT_SEQ_END();
            EXPECT_SEQ_END();
            EXPECT_SCALAR("?", 0, "mapping");
            EXPECT_MAP_START("tag:yaml.org,2002:map", 0);
            EXPECT_SCALAR("?", 0, "foo");
            EXPECT_SCALAR("?", 0, "bar");
            EXPECT_MAP_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
	}
}
