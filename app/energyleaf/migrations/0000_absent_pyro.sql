CREATE TABLE "app_config" (
	"key" text PRIMARY KEY NOT NULL,
	"value" text
);
--> statement-breakpoint
CREATE TABLE "dashboard_config" (
	"id" text PRIMARY KEY NOT NULL,
	"user_id" text NOT NULL,
	"active_components" json DEFAULT '[]'::json NOT NULL,
	CONSTRAINT "dashboard_config_user_id_unique" UNIQUE("user_id")
);
--> statement-breakpoint
CREATE TABLE "device" (
	"id" text PRIMARY KEY NOT NULL,
	"user_id" text NOT NULL,
	"name" text NOT NULL,
	"category" text NOT NULL,
	"created" timestamp with time zone DEFAULT now(),
	"timestamp" timestamp with time zone DEFAULT now() NOT NULL,
	"power" numeric(30, 6),
	"is_power_estimated" boolean DEFAULT true NOT NULL,
	"weekly_usage_estimation" numeric(30, 6)
);
--> statement-breakpoint
CREATE TABLE "device_to_peak" (
	"device_id" text NOT NULL,
	"energy_data_sequence_id" text NOT NULL,
	CONSTRAINT "device_to_peak_device_id_energy_data_sequence_id_pk" PRIMARY KEY("device_id","energy_data_sequence_id")
);
--> statement-breakpoint
CREATE TABLE "hint_config" (
	"id" text PRIMARY KEY NOT NULL,
	"user_id" text NOT NULL,
	"stage" text DEFAULT 'simple' NOT NULL,
	"stage_started_at" timestamp with time zone DEFAULT now() NOT NULL,
	"hints_enabled" boolean DEFAULT true NOT NULL,
	"hints_days_seen_in_stage" integer DEFAULT 0 NOT NULL,
	CONSTRAINT "hint_config_user_id_unique" UNIQUE("user_id")
);
--> statement-breakpoint
CREATE TABLE "hints" (
	"id" text PRIMARY KEY NOT NULL,
	"user_id" text NOT NULL,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL,
	"for_date" timestamp with time zone NOT NULL,
	"hint_type" text NOT NULL,
	"hint_stage" text NOT NULL,
	"hint_text" text NOT NULL,
	"link_target" text NOT NULL,
	"seen" boolean DEFAULT false NOT NULL,
	"seen_at" timestamp with time zone,
	"clicked" boolean DEFAULT false NOT NULL,
	"clicked_at" timestamp with time zone
);
--> statement-breakpoint
CREATE TABLE "action_logs" (
	"id" text PRIMARY KEY NOT NULL,
	"timestamp" timestamp with time zone DEFAULT now(),
	"app_function" text NOT NULL,
	"action" text NOT NULL,
	"details" json
);
--> statement-breakpoint
CREATE TABLE "error_logs" (
	"id" text PRIMARY KEY NOT NULL,
	"timestamp" timestamp with time zone DEFAULT now(),
	"app_function" text NOT NULL,
	"details" json
);
--> statement-breakpoint
CREATE TABLE "legacy_logs" (
	"id" text PRIMARY KEY NOT NULL,
	"timestamp" timestamp with time zone DEFAULT now(),
	"title" text NOT NULL,
	"log_type" text DEFAULT 'undefined',
	"app_function" text NOT NULL,
	"app_component" text DEFAULT 'undefined',
	"details" json
);
--> statement-breakpoint
CREATE TABLE "page_views" (
	"id" text PRIMARY KEY NOT NULL,
	"timestamp" timestamp with time zone DEFAULT now(),
	"path" text NOT NULL,
	"search_params" json,
	"user_agent" text,
	"user_id" text,
	"session_id" text NOT NULL,
	"params" json
);
--> statement-breakpoint
CREATE TABLE "system_logs" (
	"id" text PRIMARY KEY NOT NULL,
	"timestamp" timestamp with time zone DEFAULT now(),
	"title" text NOT NULL,
	"details" json
);
--> statement-breakpoint
CREATE TABLE "report_config" (
	"id" text PRIMARY KEY NOT NULL,
	"user_id" text NOT NULL,
	"receive_mails" boolean DEFAULT true NOT NULL,
	"anomaly" boolean DEFAULT false NOT NULL,
	"days" json DEFAULT '[]'::json
);
--> statement-breakpoint
CREATE TABLE "reports" (
	"id" text PRIMARY KEY NOT NULL,
	"timestamp" timestamp with time zone DEFAULT now() NOT NULL,
	"user_id" text NOT NULL,
	"data" json NOT NULL
);
--> statement-breakpoint
CREATE TABLE "energy_data_sequence" (
	"id" text PRIMARY KEY NOT NULL,
	"sensor_id" text NOT NULL,
	"start" timestamp with time zone NOT NULL,
	"end" timestamp with time zone NOT NULL,
	"type" text NOT NULL,
	"average_peak_power" numeric(30, 6) NOT NULL
);
--> statement-breakpoint
CREATE TABLE "energy_data" (
	"id" text NOT NULL,
	"sensor_id" text NOT NULL,
	"value" numeric(30, 6) NOT NULL,
	"consumption" numeric(30, 6) NOT NULL,
	"value_out" numeric(30, 6),
	"inserted" numeric(30, 6),
	"value_current" numeric(30, 6),
	"timestamp" timestamp with time zone DEFAULT now() NOT NULL,
	CONSTRAINT "energy_data_sensor_id_timestamp_pk" PRIMARY KEY("sensor_id","timestamp")
);
--> statement-breakpoint
CREATE TABLE "sensor_additional_user" (
	"sensor_id" text NOT NULL,
	"user_id" text NOT NULL,
	CONSTRAINT "sensor_additional_user_sensor_id_user_id_pk" PRIMARY KEY("sensor_id","user_id")
);
--> statement-breakpoint
CREATE TABLE "sensor_history" (
	"sensor_id" text NOT NULL,
	"user_id" text NOT NULL,
	"sensor_type" text NOT NULL,
	"client_id" text NOT NULL,
	CONSTRAINT "sensor_history_client_id_user_id_pk" PRIMARY KEY("client_id","user_id")
);
--> statement-breakpoint
CREATE TABLE "sensor" (
	"sensor_id" text NOT NULL,
	"client_id" text PRIMARY KEY NOT NULL,
	"version" integer DEFAULT 1 NOT NULL,
	"sensor_type" text NOT NULL,
	"user_id" text,
	"needs_script" boolean DEFAULT false NOT NULL,
	"script" text,
	CONSTRAINT "sensor_sensor_id_unique" UNIQUE("sensor_id"),
	CONSTRAINT "sensor_sensor_type_user_id_unique" UNIQUE("sensor_type","user_id")
);
--> statement-breakpoint
CREATE TABLE "sensor_token" (
	"code" text PRIMARY KEY NOT NULL,
	"sensor_id" text NOT NULL,
	"timestamp" timestamp DEFAULT now()
);
--> statement-breakpoint
CREATE TABLE "simulation_battery_settings" (
	"id" text PRIMARY KEY NOT NULL,
	"user_id" text NOT NULL,
	"enabled" boolean DEFAULT true NOT NULL,
	"capacity_kwh" numeric(30, 6) NOT NULL,
	"max_power_kw" numeric(30, 6) NOT NULL,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL,
	"updated_at" timestamp with time zone DEFAULT now() NOT NULL,
	CONSTRAINT "simulation_battery_settings_user_id_unique" UNIQUE("user_id")
);
--> statement-breakpoint
CREATE TABLE "simulation_ev_settings" (
	"id" text PRIMARY KEY NOT NULL,
	"user_id" text NOT NULL,
	"enabled" boolean DEFAULT true NOT NULL,
	"charging_speed" text NOT NULL,
	"capacity_kwh" numeric(30, 6) NOT NULL,
	"max_charging_power_kw" numeric(30, 6) DEFAULT 11 NOT NULL,
	"daily_driving_distance_km" numeric(30, 6),
	"avg_consumption_per_100km" numeric(30, 6),
	"default_schedule" json DEFAULT '[]'::json NOT NULL,
	"weekday_schedules" json DEFAULT '{}'::json NOT NULL,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL,
	"updated_at" timestamp with time zone DEFAULT now() NOT NULL,
	CONSTRAINT "simulation_ev_settings_user_id_unique" UNIQUE("user_id")
);
--> statement-breakpoint
CREATE TABLE "simulation_heat_pump_settings" (
	"id" text PRIMARY KEY NOT NULL,
	"user_id" text NOT NULL,
	"enabled" boolean DEFAULT true NOT NULL,
	"source" text NOT NULL,
	"power_kw" numeric(30, 6) NOT NULL,
	"buffer_liter" numeric(30, 6),
	"default_schedule" json DEFAULT '[]'::json NOT NULL,
	"weekday_schedules" json DEFAULT '{}'::json NOT NULL,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL,
	"updated_at" timestamp with time zone DEFAULT now() NOT NULL,
	CONSTRAINT "simulation_heat_pump_settings_user_id_unique" UNIQUE("user_id")
);
--> statement-breakpoint
CREATE TABLE "simulation_solar_settings" (
	"id" text PRIMARY KEY NOT NULL,
	"user_id" text NOT NULL,
	"enabled" boolean DEFAULT true NOT NULL,
	"peak_power" numeric(30, 6) NOT NULL,
	"orientation" text NOT NULL,
	"inverter_power" numeric(30, 6),
	"sun_hours_per_day" numeric(30, 6),
	"location" text,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL,
	"updated_at" timestamp with time zone DEFAULT now() NOT NULL,
	CONSTRAINT "simulation_solar_settings_user_id_unique" UNIQUE("user_id")
);
--> statement-breakpoint
CREATE TABLE "simulation_tou_tariff_settings" (
	"id" text PRIMARY KEY NOT NULL,
	"user_id" text NOT NULL,
	"enabled" boolean DEFAULT true NOT NULL,
	"pricing_mode" text DEFAULT 'tou' NOT NULL,
	"base_price" numeric(30, 6) NOT NULL,
	"standard_price" numeric(30, 6) NOT NULL,
	"zones" json DEFAULT '[]'::json NOT NULL,
	"weekday_zones" json DEFAULT '{}'::json NOT NULL,
	"spot_markup" numeric(30, 6) DEFAULT 3,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL,
	"updated_at" timestamp with time zone DEFAULT now() NOT NULL,
	CONSTRAINT "simulation_tou_tariff_settings_user_id_unique" UNIQUE("user_id")
);
--> statement-breakpoint
CREATE TABLE "spot_price" (
	"id" text PRIMARY KEY NOT NULL,
	"timestamp" timestamp with time zone NOT NULL,
	"price_eur_mwh" numeric(30, 6) NOT NULL,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL
);
--> statement-breakpoint
CREATE TABLE "tou_tariff_template" (
	"id" text PRIMARY KEY NOT NULL,
	"name" text NOT NULL,
	"description" text,
	"base_price" double precision NOT NULL,
	"standard_price" double precision NOT NULL,
	"zones" json DEFAULT '[]'::json NOT NULL,
	"weekday_zones" json DEFAULT '{}'::json NOT NULL,
	"is_active" boolean DEFAULT true NOT NULL,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL,
	"updated_at" timestamp with time zone DEFAULT now() NOT NULL,
	CONSTRAINT "tou_tariff_template_name_unique" UNIQUE("name")
);
--> statement-breakpoint
CREATE TABLE "session" (
	"id" text PRIMARY KEY NOT NULL,
	"user_id" text NOT NULL,
	"expires_at" timestamp with time zone NOT NULL
);
--> statement-breakpoint
CREATE TABLE "token" (
	"id" text PRIMARY KEY NOT NULL,
	"type" text,
	"user_id" text NOT NULL,
	"created_timestamp" timestamp with time zone DEFAULT now() NOT NULL
);
--> statement-breakpoint
CREATE TABLE "user_data" (
	"id" text PRIMARY KEY NOT NULL,
	"user_id" text NOT NULL,
	"timestamp" timestamp with time zone DEFAULT now() NOT NULL,
	"base_price" double precision,
	"working_price" double precision,
	"tariff" text DEFAULT 'basic',
	"household" integer,
	"property" text,
	"living_space" double precision,
	"hot_water" text,
	"advance_payment_electricity" double precision,
	"consumption_goal" double precision,
	"electricity_meter_number" text,
	"electricity_meter_type" text,
	"electricity_meter_img_url" text,
	"power_at_electricity_meter" boolean DEFAULT false,
	"wifi_at_electricity_meter" boolean DEFAULT false,
	"installation_comment" text,
	"device_power_estimation_r_squared" double precision,
	"current_energy_threshold" numeric(30, 6),
	"show_solar_feedin" boolean DEFAULT false NOT NULL
);
--> statement-breakpoint
CREATE TABLE "user_experiment_data" (
	"user_id" text PRIMARY KEY NOT NULL,
	"experiment_status" text,
	"installation_date" timestamp with time zone,
	"deinstallation_date" timestamp with time zone,
	"experiment_number" integer,
	"gets_paid" boolean DEFAULT false NOT NULL,
	"uses_prolific" boolean DEFAULT false NOT NULL
);
--> statement-breakpoint
CREATE TABLE "user" (
	"id" text PRIMARY KEY NOT NULL,
	"created" timestamp with time zone DEFAULT now(),
	"phone" text,
	"address" text DEFAULT '' NOT NULL,
	"firstname" text DEFAULT '' NOT NULL,
	"lastname" text DEFAULT '' NOT NULL,
	"username" text NOT NULL,
	"email" text NOT NULL,
	"password" text NOT NULL,
	"is_admin" boolean DEFAULT false NOT NULL,
	"is_active" boolean DEFAULT false NOT NULL,
	"is_participant" boolean DEFAULT false NOT NULL,
	"is_simulation_free" boolean DEFAULT false NOT NULL,
	"onboarding_completed" boolean DEFAULT false NOT NULL,
	"activation_date" timestamp with time zone,
	"deleted" boolean DEFAULT false NOT NULL,
	"user_timezone" text DEFAULT 'europe_berlin',
	CONSTRAINT "user_email_unique" UNIQUE("email")
);
--> statement-breakpoint
ALTER TABLE "dashboard_config" ADD CONSTRAINT "dashboard_config_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "device" ADD CONSTRAINT "device_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "device_to_peak" ADD CONSTRAINT "device_to_peak_energy_data_sequence_id_energy_data_sequence_id_fk" FOREIGN KEY ("energy_data_sequence_id") REFERENCES "public"."energy_data_sequence"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "hint_config" ADD CONSTRAINT "hint_config_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "hints" ADD CONSTRAINT "hints_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "report_config" ADD CONSTRAINT "report_config_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "reports" ADD CONSTRAINT "reports_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "sensor_additional_user" ADD CONSTRAINT "sensor_additional_user_sensor_id_sensor_sensor_id_fk" FOREIGN KEY ("sensor_id") REFERENCES "public"."sensor"("sensor_id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "sensor_additional_user" ADD CONSTRAINT "sensor_additional_user_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "sensor" ADD CONSTRAINT "sensor_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE set null ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "sensor_token" ADD CONSTRAINT "sensor_token_sensor_id_sensor_sensor_id_fk" FOREIGN KEY ("sensor_id") REFERENCES "public"."sensor"("sensor_id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "simulation_battery_settings" ADD CONSTRAINT "simulation_battery_settings_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "simulation_ev_settings" ADD CONSTRAINT "simulation_ev_settings_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "simulation_heat_pump_settings" ADD CONSTRAINT "simulation_heat_pump_settings_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "simulation_solar_settings" ADD CONSTRAINT "simulation_solar_settings_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "simulation_tou_tariff_settings" ADD CONSTRAINT "simulation_tou_tariff_settings_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "session" ADD CONSTRAINT "session_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "token" ADD CONSTRAINT "token_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "user_data" ADD CONSTRAINT "user_data_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
CREATE UNIQUE INDEX "senor_data_sequence_sensor_id_start" ON "energy_data_sequence" USING btree ("sensor_id","start");--> statement-breakpoint
CREATE UNIQUE INDEX "senor_data_sequence_sensor_id_end" ON "energy_data_sequence" USING btree ("sensor_id","end");--> statement-breakpoint
CREATE INDEX "senor_data_sequence_sensor_id" ON "energy_data_sequence" USING btree ("sensor_id","start","end");--> statement-breakpoint
CREATE INDEX "sensor_id_idx_sensor_token" ON "sensor_token" USING btree ("sensor_id");--> statement-breakpoint
CREATE UNIQUE INDEX "spot_price_timestamp_idx" ON "spot_price" USING btree ("timestamp");--> statement-breakpoint
CREATE INDEX "spot_price_timestamp_range_idx" ON "spot_price" USING btree ("timestamp");