﻿using System;
using System.Collections.Generic;

namespace XForm.Data
{
    public interface IDataBatchEnumerator : IDisposable
    {
        /// <summary>
        ///  Get the columns available from this source.
        /// </summary>
        IReadOnlyList<ColumnDetails> Columns { get; }

        /// <summary>
        ///  Go back to the first rows from this source again.
        /// </summary>
        void Reset();

        /// <summary>
        ///  Request the next batch of rows from the source of up to desiredCount rows.
        /// </summary>
        /// <param name="desiredCount">Desired Row count; fewer or more rows may be returned.</param>
        /// <returns>Row count returned, zero if no more rows available</returns>
        int Next(int desiredCount);

        /// <summary>
        ///  Return a function which returns the DataBatch for the desired column
        ///  for the current batch of rows.
        /// </summary>
        /// <param name="columnIndex">Index of column to provide a getter for</param>
        /// <returns>Function which will return the DataBatch for columnName for the current row batch on each call</returns>
        Func<DataBatch> ColumnGetter(int columnIndex);
    }

    public interface IDataBatchList : IDataBatchEnumerator
    {
        /// <summary>
        ///  Get the total row count of this list.
        /// </summary>
        int Count { get; }

        /// <summary>
        ///  Get a specific set of rows.
        /// </summary>
        /// <param name="indices">Indices of rows to get, zero based relative to the full set</param>
        /// <param name="indexCount">Count of indices which are valid in indices array</param>
        /// <returns>Number of rows retrieved</returns>
        int Next(int[] indices, int indexCount);
    }
}