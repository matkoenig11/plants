from __future__ import annotations

import threading
import tkinter as tk
from tkinter import ttk

from garden_api_client import (
    PlantCandidate,
    build_detailed_report,
    fetch_candidate_details,
    get_provider_definition,
    load_provider_base_url,
    load_provider_token,
    provider_choices,
    search_best_fit,
)


class GardenApiBrowser(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("Garden API Tester")
        self.geometry("1180x780")
        self.minsize(980, 680)

        providers = provider_choices()
        self.provider_map = {provider.display_name: provider.provider_id for provider in providers}
        self.query_var = tk.StringVar()
        self.provider_var = tk.StringVar(value=providers[0].display_name)
        self.token_var = tk.StringVar()
        self.base_url_var = tk.StringVar()
        self.status_var = tk.StringVar(value="Choose a provider, enter a plant name, then search.")
        self.note_var = tk.StringVar()
        self.token_label_var = tk.StringVar()
        self.base_url_label_var = tk.StringVar(value="Base URL")
        self.candidates_by_slug: dict[str, PlantCandidate] = {}

        self._build_ui()
        self.on_provider_changed()

    def _build_ui(self) -> None:
        root = ttk.Frame(self, padding=16)
        root.pack(fill=tk.BOTH, expand=True)
        root.columnconfigure(0, weight=2)
        root.columnconfigure(1, weight=3)
        root.rowconfigure(3, weight=1)

        title = ttk.Label(root, text="Garden API Search", font=("Segoe UI", 18, "bold"))
        title.grid(row=0, column=0, columnspan=2, sticky="w")

        form = ttk.Frame(root)
        form.grid(row=1, column=0, columnspan=2, sticky="ew", pady=(12, 12))
        form.columnconfigure(1, weight=1)

        ttk.Label(form, text="Provider").grid(row=0, column=0, sticky="w", padx=(0, 10), pady=(0, 8))
        provider_combo = ttk.Combobox(
            form,
            textvariable=self.provider_var,
            values=list(self.provider_map.keys()),
            state="readonly",
        )
        provider_combo.grid(row=0, column=1, sticky="ew", pady=(0, 8))
        provider_combo.bind("<<ComboboxSelected>>", lambda _event: self.on_provider_changed())

        ttk.Label(form, text="Plant name").grid(row=1, column=0, sticky="w", padx=(0, 10), pady=(0, 8))
        query_entry = ttk.Entry(form, textvariable=self.query_var)
        query_entry.grid(row=1, column=1, sticky="ew", pady=(0, 8))
        query_entry.bind("<Return>", lambda _event: self.start_search())

        self.token_label = ttk.Label(form, textvariable=self.token_label_var)
        self.token_entry = ttk.Entry(form, textvariable=self.token_var, show="*")
        self.token_label.grid(row=2, column=0, sticky="w", padx=(0, 10), pady=(0, 8))
        self.token_entry.grid(row=2, column=1, sticky="ew", pady=(0, 8))

        self.base_url_label = ttk.Label(form, textvariable=self.base_url_label_var)
        self.base_url_entry = ttk.Entry(form, textvariable=self.base_url_var)
        self.base_url_label.grid(row=3, column=0, sticky="w", padx=(0, 10))
        self.base_url_entry.grid(row=3, column=1, sticky="ew")

        note_label = ttk.Label(form, textvariable=self.note_var, wraplength=820, foreground="#666666")
        note_label.grid(row=4, column=0, columnspan=2, sticky="w", pady=(8, 0))

        actions = ttk.Frame(root)
        actions.grid(row=2, column=0, columnspan=2, sticky="ew", pady=(0, 12))

        self.search_button = ttk.Button(actions, text="Search Best Fit", command=self.start_search)
        self.search_button.pack(side=tk.LEFT)

        ttk.Label(actions, textvariable=self.status_var).pack(side=tk.LEFT, padx=(12, 0))

        left_panel = ttk.Frame(root)
        left_panel.grid(row=3, column=0, sticky="nsew", padx=(0, 12))
        left_panel.columnconfigure(0, weight=1)
        left_panel.rowconfigure(1, weight=1)

        ttk.Label(left_panel, text="Returned possibilities").grid(row=0, column=0, sticky="w")

        self.candidate_tree = ttk.Treeview(
            left_panel,
            columns=("common_name", "scientific_name", "family", "score"),
            show="headings",
            selectmode="browse",
        )
        self.candidate_tree.heading("common_name", text="Common name")
        self.candidate_tree.heading("scientific_name", text="Scientific name")
        self.candidate_tree.heading("family", text="Family")
        self.candidate_tree.heading("score", text="Score")
        self.candidate_tree.column("common_name", width=170, anchor=tk.W)
        self.candidate_tree.column("scientific_name", width=210, anchor=tk.W)
        self.candidate_tree.column("family", width=150, anchor=tk.W)
        self.candidate_tree.column("score", width=70, anchor=tk.CENTER)
        self.candidate_tree.grid(row=1, column=0, sticky="nsew")
        self.candidate_tree.bind("<<TreeviewSelect>>", self.on_candidate_selected)

        candidate_scroll = ttk.Scrollbar(left_panel, orient=tk.VERTICAL, command=self.candidate_tree.yview)
        candidate_scroll.grid(row=1, column=1, sticky="ns")
        self.candidate_tree.configure(yscrollcommand=candidate_scroll.set)

        right_panel = ttk.Frame(root)
        right_panel.grid(row=3, column=1, sticky="nsew")
        right_panel.columnconfigure(0, weight=1)
        right_panel.rowconfigure(1, weight=1)

        ttk.Label(right_panel, text="Selected plant details").grid(row=0, column=0, sticky="w")
        self.summary_text = tk.Text(right_panel, wrap=tk.WORD)
        self.summary_text.grid(row=1, column=0, sticky="nsew", pady=(4, 0))

        summary_scroll = ttk.Scrollbar(right_panel, orient=tk.VERTICAL, command=self.summary_text.yview)
        summary_scroll.grid(row=1, column=1, sticky="ns")
        self.summary_text.configure(yscrollcommand=summary_scroll.set)

    def current_provider_id(self) -> str:
        return self.provider_map[self.provider_var.get()]

    def current_provider(self):
        return get_provider_definition(self.current_provider_id())

    def on_provider_changed(self) -> None:
        provider = self.current_provider()
        self.note_var.set(provider.notes)
        self.token_label_var.set(provider.token_label or "API token")
        self.token_var.set(load_provider_token(provider.provider_id))
        self.base_url_var.set(load_provider_base_url(provider.provider_id))

        token_state = tk.NORMAL if provider.requires_token else tk.DISABLED
        base_url_state = tk.NORMAL if provider.supports_custom_base_url else tk.DISABLED

        self.token_entry.configure(state=token_state)
        self.base_url_entry.configure(state=base_url_state)
        self.token_label.configure(foreground="black" if provider.requires_token else "#888888")
        self.base_url_label.configure(foreground="black" if provider.supports_custom_base_url else "#888888")

    def set_busy(self, busy: bool, message: str) -> None:
        self.search_button.configure(state=tk.DISABLED if busy else tk.NORMAL)
        self.status_var.set(message)

    def start_search(self) -> None:
        query = self.query_var.get().strip()
        provider_id = self.current_provider_id()
        token = self.token_var.get().strip()
        base_url = self.base_url_var.get().strip()

        self.set_busy(True, f"Searching {get_provider_definition(provider_id).display_name}...")
        self.summary_text.delete("1.0", tk.END)
        self.clear_candidates()

        thread = threading.Thread(
            target=self._run_search,
            args=(query, provider_id, token, base_url),
            daemon=True,
        )
        thread.start()

    def clear_candidates(self) -> None:
        self.candidates_by_slug.clear()
        for item_id in self.candidate_tree.get_children():
            self.candidate_tree.delete(item_id)

    def _run_search(self, query: str, provider_id: str, token: str, base_url: str) -> None:
        try:
            best, details, candidates = search_best_fit(query, provider_id, token=token, base_url=base_url)
            self.after(0, lambda: self._show_candidates(candidates, best.slug, details))
        except Exception as exc:  # noqa: BLE001
            self.after(0, lambda: self._show_error(str(exc)))

    def _show_candidates(self, candidates: list[PlantCandidate], initial_slug: str, initial_details: dict) -> None:
        self.clear_candidates()
        for candidate in candidates:
            self.candidates_by_slug[candidate.slug] = candidate
            self.candidate_tree.insert(
                "",
                tk.END,
                iid=candidate.slug,
                values=(
                    candidate.common_name or candidate.scientific_name or candidate.slug,
                    candidate.scientific_name or "Unknown",
                    candidate.family or candidate.family_common_name or "Unknown",
                    f"{candidate.score:.0f}",
                ),
            )

        if initial_slug in self.candidates_by_slug:
            self.candidate_tree.selection_set(initial_slug)
            self.candidate_tree.focus(initial_slug)
            self.candidate_tree.see(initial_slug)
            self._show_candidate_details(self.candidates_by_slug[initial_slug], initial_details)

        self.set_busy(False, "Search completed. Click any possibility for full details.")

    def on_candidate_selected(self, _event: object) -> None:
        selected = self.candidate_tree.selection()
        if not selected:
            return

        slug = selected[0]
        candidate = self.candidates_by_slug.get(slug)
        if not candidate:
            return

        token = self.token_var.get().strip()
        base_url = self.base_url_var.get().strip()
        self.summary_text.delete("1.0", tk.END)
        self.summary_text.insert("1.0", f"Loading full details for {candidate.common_name or candidate.scientific_name}...")
        self.status_var.set("Fetching plant details...")

        thread = threading.Thread(
            target=self._load_candidate_details,
            args=(candidate, token, base_url),
            daemon=True,
        )
        thread.start()

    def _load_candidate_details(self, candidate: PlantCandidate, token: str, base_url: str) -> None:
        try:
            details = fetch_candidate_details(candidate, token=token, base_url=base_url)
            self.after(0, lambda: self._show_candidate_details(candidate, details))
        except Exception as exc:  # noqa: BLE001
            self.after(0, lambda: self._show_error(str(exc)))

    def _show_candidate_details(self, candidate: PlantCandidate, details: dict) -> None:
        report = build_detailed_report(candidate, details)
        self.summary_text.delete("1.0", tk.END)
        self.summary_text.insert("1.0", report)
        provider = get_provider_definition(candidate.provider_id)
        self.status_var.set(f"Loaded full details from {provider.display_name}.")

    def _show_error(self, message: str) -> None:
        self.summary_text.delete("1.0", tk.END)
        self.summary_text.insert("1.0", message)
        self.set_busy(False, "Request failed.")


if __name__ == "__main__":
    app = GardenApiBrowser()
    app.mainloop()
