.mode tabs
.out speller.tab
.header off
select word,min(onum) from speller where onum is not null group by word;
