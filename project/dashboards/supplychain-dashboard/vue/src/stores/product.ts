import { defineStore } from 'pinia'
import http from '@/utils/http'

export interface ProductComponent {
  component: number
  component_key: string
  quantity: number
}

export interface Product {
  id: number
  product_key: string
  product_type: number | null
  batch: string
  owner: number | null
  created_timestamp: string
  components: ProductComponent[]
  recorded_by: number | null
}

interface ProductState {
  product: Product | null
  loading: boolean
  error: string | null
}

export const useProductStore = defineStore('product', {
  state: (): ProductState => ({
    product: null,
    loading: false,
    error: null,
  }),

  actions: {
    async fetchProduct(id: string | number) {
      this.loading = true
      this.error = null

      try {
        const { data } = await http.get<Product>(`/api/products/${id}/`)
        this.product = data
      } catch (err) {
        this.error = err instanceof Error ? err.message : String(err)
      } finally {
        this.loading = false
      }
    },
  },
})

