-- Claims triangle storage + convenience views for downstream queries.
CREATE TABLE IF NOT EXISTS claims_triangle (
    origin_year INTEGER NOT NULL,
    dev_year    INTEGER NOT NULL,
    paid        REAL    NOT NULL,
    PRIMARY KEY (origin_year, dev_year)
);

CREATE INDEX IF NOT EXISTS idx_claims_origin ON claims_triangle(origin_year);

-- Upper triangle (only the known diagonal+above)
CREATE VIEW IF NOT EXISTS upper_triangle AS
SELECT origin_year, dev_year, paid
FROM claims_triangle
WHERE origin_year + dev_year < (SELECT MAX(origin_year) + 1 FROM claims_triangle);

-- Column totals — mirror of the C++ colSum computation, for sanity checks
CREATE VIEW IF NOT EXISTS col_totals AS
SELECT dev_year AS j, SUM(paid) AS col_sum, COUNT(*) AS n_rows
FROM claims_triangle
GROUP BY dev_year
ORDER BY dev_year;
