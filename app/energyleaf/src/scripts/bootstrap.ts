import { randomBytes } from "node:crypto";
import { hash } from "@node-rs/argon2";
import { eq } from "drizzle-orm";
import { genID } from "@/lib/utils";
import { db } from "@/server/db";
import { userTable } from "@/server/db/tables/user";

// usage: bun run src/scripts/bootstrap.ts --email=<admin email> [--password=<password>]
// creates the initial admin user if missing. idempotent: safe to re-run.
// sensors are created/attached through the application; a premade sensor ships in the ready-to-bake image.

function argValue(name: string): string | undefined {
	const arg = process.argv.find((a) => a.startsWith(`--${name}=`));
	return arg ? arg.slice(arg.indexOf("=") + 1) : undefined;
}

async function main() {
	const email = argValue("email");
	if (!email) {
		console.error("Usage: bun run src/scripts/bootstrap.ts --email=<admin email> [--password=<password>]");
		process.exit(1);
	}
	const password = argValue("password") ?? randomBytes(18).toString("base64url");
	const generatedPassword = !argValue("password");

	const existingUser = await db
		.select({ id: userTable.id })
		.from(userTable)
		.where(eq(userTable.email, email))
		.limit(1);
	if (existingUser.length > 0) {
		console.log(`User ${email} already exists, nothing to do.`);
		return;
	}

	await db.insert(userTable).values({
		id: genID(30),
		username: email,
		email,
		password: await hash(password),
		isAdmin: true,
		// login requires isActive
		isActive: true,
		onboardingCompleted: true,
	});
	console.log(`Created admin user ${email}.`);

	console.log("\n=== Bootstrap summary ===");
	console.log(`Admin email: ${email}`);
	if (generatedPassword) {
		console.log(`Password:    ${password}`);
		console.log("(shown once, store it now)");
	}
}

main()
	.then(() => process.exit(0))
	.catch((err) => {
		console.error(err);
		process.exit(1);
	});
