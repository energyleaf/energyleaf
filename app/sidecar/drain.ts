import { drizzle } from "drizzle-orm/node-postgres";
import { sql } from "drizzle-orm";

const SECRET = process.env.CRON_SECRET!;
const APP = process.env.API_URL!;
const db = drizzle(process.env.DATABASE_URL!);

async function enqueue(path: string) {
    const res = await fetch(`${APP}${path}`, {
        headers: { "Authorization": `Bearer ${SECRET}` }
    });
    console.log(`GET ${path}: ${res.status}`);
}

async function drain(path: string, queue: string) {
    const messages = await db.execute(
        sql`SELECT * FROM pgmq.read(${queue}::text, 300, 50)`
    );

    for (const row of messages.rows) {
        const res = await fetch(`${APP}${path}`, {
            method: "POST",
            headers: {
                "Authorization": `Bearer ${SECRET}`,
                "Content-Type": "application/json",
            },
            body: JSON.stringify(row),
        });
        console.log(`POST ${path}: ${res.status}`);
    }
}

const cmd = process.argv[2];

if (cmd === "enqueue-peaks") await enqueue("/peaks");
if (cmd === "enqueue-anomalies") await enqueue("/anomalies");
if (cmd === "enqueue-reports") await enqueue("/reports");
if (cmd === "request-spot") await enqueue("/spot-prices");
if (cmd === "enqueue-hints") await enqueue("/hints");

if (cmd === "drain-peaks") await drain("/peaks", "peaks_queue");
if (cmd === "drain-anomalies") await drain("/anomalies", "anomalies_queue");
if (cmd === "drain-reports") await drain("/reports", "reports_queue");

process.exit(0);