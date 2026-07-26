Suite plots
===========

This page shows a 2D visualization of 500 functions from the packaged
``2026_v1`` suite. The plot uses log-scaled function values so broad
landscape structure remains visible across functions with different value
ranges.

.. raw:: html

   <iframe
     src="_static/2D_plot_log.pdf"
     width="100%"
     height="760px"
     style="border: 1px solid #d0d7de; border-radius: 4px;">
     This browser does not support embedded PDFs.
   </iframe>

If the PDF does not render in your browser, download it directly:
:download:`2D_plot_log.pdf <figs/2D_plot_log.pdf>`.

The suite itself is available through:

.. code-block:: python

   import funccraft as fc

   year = 2026
   version = 1
   suite = fc.suite_collection(year, version).benchmark_suite(2)

and in C++:

.. code-block:: cpp

   const int year = 2026;
   const int version = 1;
   FuncCraft::BenchmarkSuite suite =
       FuncCraft::suite_collection(year, version).benchmark_suite(2);
