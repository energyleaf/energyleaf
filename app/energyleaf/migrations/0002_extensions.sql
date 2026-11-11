CREATE TABLE IF NOT EXISTS public.app_config (
    key TEXT PRIMARY KEY,
    value TEXT
);--> statement-breakpoint
SELECT pgmq.create('peaks_queue');--> statement-breakpoint
SELECT pgmq.create('reports_queue');--> statement-breakpoint
SELECT pgmq.create('anomalies_queue');