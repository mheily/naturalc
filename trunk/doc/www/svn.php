<?php include "header.html"?>

<h2>Subversion</h2>

The source code for Natural C is kept in a Subversion repository.

<p>
You can <a href="http://naturalc.googlecode.com/svn/trunk/">browse the repository online</a>, or download a copy using the Subversion client.
<p>
To download a working copy of the repository, issue the following command:
<p>
<pre>
svn checkout http://naturalc.googlecode.com/svn/trunk/ ncc
</pre>
<p>
The following sequence of commands will download, build, and install a copy of Natural C:
<p>
<pre>
svn checkout http://naturalc.googlecode.com/svn/trunk/ ncc
cd ncc
autoreconf -f -i
./configure
make
make install
</pre>

<?php include "footer.html"?>
