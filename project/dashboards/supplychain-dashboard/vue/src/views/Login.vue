<script setup lang="ts">
import { ref } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { useForm } from 'vee-validate'
import { toTypedSchema } from '@vee-validate/zod'
import * as z from 'zod'
import http from '@/utils/http'
import { login } from '@/services/auth'
import { useAuthStore } from '@/stores/auth'

import { cn } from '@/lib/utils'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { FormControl, FormField, FormItem, FormLabel, FormMessage } from '@/components/ui/form'
import { Package } from 'lucide-vue-next'

const formSchema = toTypedSchema(z.object({
  email: z.string().email('Enter a valid email'),
  password: z.string().min(1, 'Password is required'),
}))

const form = useForm({
  validationSchema: formSchema,
})

const loginError = ref<string | null>(null)
const router = useRouter()
const route = useRoute()

const onSubmit = form.handleSubmit(async (values) => {
  loginError.value = null
  try {
    await login(values.email, values.password)
    const auth = useAuthStore()
    await auth.fetchUser()
    const redirect = (route.query.redirect as string) || '/dashboard'
    router.push(redirect)
  } catch (err) {
    loginError.value = 'Login failed'
  }
})

const loginAsGuest = async () => {
  loginError.value = null
  try {
    const { data } = await http.post('/api/create_temporary_user/')

    const { username, password } = await data

    await login(username, password)

    const auth = useAuthStore()
    await auth.fetchUser()
    const redirect = (route.query.redirect as string) || '/dashboard'

    router.push(redirect)
  } catch (err) {
    loginError.value = 'Login failed'
  }
}

</script>

<template>
  <div class="grid min-h-svh lg:grid-cols-2">
    <!-- Left Column: Form -->
    <div class="flex flex-col gap-4 p-6 md:p-10">
      <!-- Logo -->
      <div class="flex justify-center gap-2 md:justify-start">
        <a href="/" class="flex items-center font-medium">
          <h2 class="text-2xl text-primary font-semibold flex items-center">
            <span class="mr-2 flex items-center">
              <Icon name="Package" />
            </span>PathLedger
          </h2>
        </a>
      </div>

      <!-- Login Form -->
      <div class="flex flex-1 items-center justify-center">
        <div class="w-full max-w-xs">
          <form :class="cn('flex flex-col gap-6')" @submit.prevent="onSubmit">
            <div class="flex flex-col items-center gap-2 text-center">
              <h1 class="text-2xl font-bold">
                Login to your account
              </h1>
              <p class="text-balance text-sm text-muted-foreground">
                Enter your email below to login to your account
              </p>
            </div>
            <div class="grid gap-6">
              <FormField v-slot="{ componentField }" name="email">
                <FormItem class="grid gap-2">
                  <FormLabel>Email</FormLabel>
                  <FormControl>
                    <Input type="email" placeholder="m@example.com" v-bind="componentField" required />
                  </FormControl>
                  <FormMessage />
                </FormItem>
              </FormField>

              <FormField v-slot="{ componentField }" name="password">
                <FormItem class="grid gap-2">
                  <div class="flex items-center">
                    <FormLabel>Password</FormLabel>
                    <a href="#" class="ml-auto text-sm underline-offset-4 hover:underline">
                      Forgot your password?
                    </a>
                  </div>
                  <FormControl>
                    <Input type="password" v-bind="componentField" required />
                  </FormControl>
                  <FormMessage />
                </FormItem>
              </FormField>

              <p v-if="loginError" class="text-red-500 mt-2">{{ loginError }}</p>

              <Button type="submit" class="w-full">Login</Button>

              <div
                class="relative text-center text-sm after:absolute after:inset-0 after:top-1/2 after:z-0 after:flex after:items-center after:border-t after:border-border">
                <span class="relative z-10 bg-background px-2 text-muted-foreground">
                  Or
                </span>
              </div>

              <Button variant="outline" class="w-full" @click="loginAsGuest">
                Continue as Guest
              </Button>
            </div>
            <div class="text-center text-sm">
              Don't have an account?
              <a href="#" class="underline underline-offset-4">
                Sign up
              </a>
            </div>
          </form>
          <div class="flex items-center justify-center mt-4">
              <img class="w-1/2 p-2" src="/assets/images/university-of-queensland.svg" alt="University of Queensland">
          </div>
        </div>
      </div>
    </div>

    <!-- Right Column: Image -->
    <div class="relative hidden bg-muted lg:block">
      <div class="h-full w-full bg-cover bg-center filter brightness-50 saturate-0 transform animate-slow-zoom"
        :style="{ backgroundImage: 'url(/assets/images/landingpage/landingpage-bg-2.jpg)' }"></div>
      <div class="absolute inset-0 bg-accent opacity-30"></div>
    </div>
  </div>
</template>
